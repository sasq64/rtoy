
#include "rsprites.hpp"

#include "gl/buffer.hpp"
#include "mrb_tools.hpp"
#include "rimage.hpp"

#include <gl/gl.hpp>
#include <gl/program_cache.hpp>
#include <pix/pix.hpp>

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <mruby/class.h>

#include <algorithm>

mrb_data_type RSprite::dt{"Sprite", [](mrb_state*, void* data) {}};
mrb_data_type RSprites::dt{"Sprites", [](mrb_state*, void* data) {}};

void RSprite::update_tx(float screen_width, float screen_height)
{
    glm::mat4x4 m(1.0F);
    m = glm::scale(m, glm::vec3(2.0 / screen_width, 2.0 / screen_height, 1.0));
    m = glm::translate(m, glm::vec3(-screen_width / 2.0 + trans[0],
                              screen_height / 2.0 - trans[1], 0));
    m = glm::rotate(m, rot, glm::vec3(0.0, 0.0, 1.0));
    m = glm::translate(
        m, glm::vec3(width / 2 * scale[0], -height / 2 * scale[1], 0));
    m = glm::scale(m, glm::vec3(static_cast<float>(width) * scale[0] / 2,
                          static_cast<float>(height) * scale[1] / -2, 1.0));
    memcpy(transform.data(), glm::value_ptr(m), 16 * 4);
}

RSprites::RSprites(int w, int h) : RLayer{w, h} {}

void RSprites::reset()
{
    RLayer::reset();
    batches.clear();
}

void RSprites::clear()
{
    batches.clear();
}

void RSprites::render()
{
    //if (batches.empty()) { return; }
    glEnable(GL_BLEND);
    glLineWidth(style.line_width);
    pix::set_colors(style.fg, style.bg);
    // pix::set_transform(transform);
    auto& textured = gl::ProgramCache::get_instance().textured;
    textured.use();
    float last_alpha = -1;
    auto pos = textured.getAttribute("in_pos");
    auto uv = textured.getAttribute("in_uv");
    pos.enable();
    uv.enable();

    auto draw_batch = [&](SpriteBatch& batch) {
        batch.texture->bind();
        for (auto const& sprite : batch.sprites) {
            if (last_alpha != sprite->alpha) {
                gl::Color fg = style.fg;
                fg.alpha = sprite->alpha;
                textured.setUniform("in_color", fg);
                last_alpha = sprite->alpha;
            }
            if (sprite->dirty) {
                sprite->dirty = false;
                sprite->update_tx(width, height);
            }
            textured.setUniform("in_transform", sprite->transform);
            sprite->vbo.bind();
            gl::vertexAttrib(pos, 2, gl::Type::Float, 0 * sizeof(GLfloat), 0);
            gl::vertexAttrib(
                uv, 2, gl::Type::Float, 0 * sizeof(GLfloat), 8 * 4);
            gl::drawArrays(gl::Primitive::TriangleFan, 0, 4);
        }
    };

    for (auto& [_, batch] : batches) {
        draw_batch(batch);
    }
    if(fixed_batch.texture != nullptr) {
        draw_batch(fixed_batch);
    }
    pos.disable();
    uv.disable();
}

RSprite* RSprites::add_sprite(RImage* image, int flags)
{
    image->upload();

    auto& batch = flags == 1 ? fixed_batch : batches[image->texture.tex->tex_id];
    if (batch.texture == nullptr) {
        batch.texture = image->texture.tex;
        batch.image = image->image;
    }
    auto& uvs = image->texture.uvs;
    std::array vertexData{-1.F, -1.F, 1.F, -1.F, 1.F, 1.F, -1.F, 1.F, 0.F, 0.F,
        1.F, 0.F, 1.F, 1.F, 0.F, 1.F};
    std::copy(uvs.begin(), uvs.end(), vertexData.begin() + 8);

    batch.sprites.push_back(
        new RSprite{gl_wrap::ArrayBuffer<GL_STATIC_DRAW>{vertexData},
            static_cast<float>(width), static_cast<float>(height)});
    auto* spr = batch.sprites.back();
    spr->parent = &batch;
    spr->uvs = image->texture.uvs;
    spr->width = image->width();
    spr->height = image->height();
    spr->trans[0] = image->x();
    spr->trans[1] = image->y();

    spr->update_tx(width, height);
    return spr;
}

void RSprites::remove_sprite(RSprite* spr)
{
    // TODO: Optimize
    auto& sprites = spr->parent->sprites;
    sprites.erase(
        std::remove(sprites.begin(), sprites.end(), spr), sprites.end());
    if (sprites.empty()) {
        batches.erase(spr->parent->texture->tex_id);
        //spr->parent->texture = nullptr;
        fmt::print("Removing batch, {} left\n", batches.size());
    }
}

void RSprites::reg_class(mrb_state* ruby)
{
    rclass = mrb_define_class(ruby, "Sprites", RLayer::rclass);
    MRB_SET_INSTANCE_TT(RSprites::rclass, MRB_TT_DATA);

    RSprite::rclass = mrb_define_class(ruby, "Sprite", ruby->object_class);
    MRB_SET_INSTANCE_TT(RSprite::rclass, MRB_TT_DATA);

    mrb_define_method(
        ruby, RSprites::rclass, "add_sprite",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* ptr = mrb::self_to<RSprites>(self);
            RImage* image = nullptr;
            mrb_get_args(mrb, "d", &image, &RImage::dt);
            auto* spr = ptr->add_sprite(image, 0);
            return mrb::new_data_obj(mrb, spr);
        },
        MRB_ARGS_REQ(3));

    mrb_define_method(
        ruby, RSprites::rclass, "remove_sprite",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* ptr = mrb::self_to<RSprites>(self);
            RSprite* spr = nullptr;
            mrb_get_args(mrb, "d", &spr, &RSprite::dt);
            ptr->remove_sprite(spr);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(3));

    mrb_define_method(
        ruby, RSprite::rclass, "y=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [y] = mrb::get_args<float>(mrb);
            auto* rspr = mrb::self_to<RSprite>(self);
            rspr->trans[1] = y;
            rspr->dirty = true;
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    mrb_define_method(
        ruby, RSprite::rclass, "y",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rspr = mrb::self_to<RSprite>(self);
            return mrb::to_value(rspr->trans[1], mrb);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, RSprite::rclass, "x=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [x] = mrb::get_args<float>(mrb);
            auto* rspr = mrb::self_to<RSprite>(self);
            rspr->trans[0] = x;
            rspr->dirty = true;
            // rspr->pos.first = x;
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));
    mrb_define_method(
        ruby, RSprite::rclass, "x",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rspr = mrb::self_to<RSprite>(self);
            return mrb::to_value(rspr->trans[0], mrb);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, RSprite::rclass, "img",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rspr = mrb::self_to<RSprite>(self);
            auto* rimage = new RImage(rspr->parent->image);
            rimage->texture.tex = rspr->parent->texture;
            rimage->texture.uvs = rspr->uvs;
            return mrb::new_data_obj(mrb, rimage);
        },
        MRB_ARGS_NONE());

    /* mrb_define_method( */
    /*     ruby, RSprite::rclass, "img=", */
    /*     [](mrb_state* mrb, mrb_value self) -> mrb_value { */
    /*         auto* spr = mrb::self_to<RSprite>(self); */
    /*         RImage* image = nullptr; */
    /*         mrb_get_args(mrb, "d", &image, &RImage::dt); */
    /*         image->upload(); */
    /*         spr->image = image->image; */
    /*         spr->texture = image->texture; */
    /*         spr->width = image->width(); */
    /*         spr->height = image->height(); */
    /*         spr->dirty = true; */
    /*         return mrb_nil_value(); */
    /*     }, */
    /*     MRB_ARGS_REQ(1)); */

    mrb_define_method(
        ruby, RSprite::rclass, "alpha=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [x] = mrb::get_args<float>(mrb);
            auto* rspr = mrb::self_to<RSprite>(self);
            rspr->alpha = x;
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));
    mrb_define_method(
        ruby, RSprite::rclass, "alpha",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rspr = mrb::self_to<RSprite>(self);
            return mrb::to_value(rspr->alpha, mrb);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, RSprite::rclass, "scalex=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [x] = mrb::get_args<float>(mrb);
            auto* rspr = mrb::self_to<RSprite>(self);
            rspr->scale[0] = x;
            rspr->dirty = true;
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));
    mrb_define_method(
        ruby, RSprite::rclass, "scaley=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [y] = mrb::get_args<float>(mrb);
            auto* rspr = mrb::self_to<RSprite>(self);
            rspr->scale[1] = y;
            rspr->dirty = true;
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));
    mrb_define_method(
        ruby, RSprite::rclass, "scale=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [x] = mrb::get_args<float>(mrb);
            auto* rspr = mrb::self_to<RSprite>(self);
            rspr->scale[0] = rspr->scale[1] = x;
            rspr->dirty = true;
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));
    mrb_define_method(
        ruby, RSprite::rclass, "scale",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rspr = mrb::self_to<RSprite>(self);
            return mrb::to_value(rspr->scale[0], mrb);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, RSprite::rclass, "set_scale",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [x, y] = mrb::get_args<float, float>(mrb);
            auto* rspr = mrb::self_to<RSprite>(self);
            rspr->scale[0] = x;
            rspr->scale[1] = y;
            rspr->dirty = true;
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(2));

    mrb_define_method(
        ruby, RSprite::rclass, "rotation=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [x] = mrb::get_args<float>(mrb);
            auto* rsprite = mrb::self_to<RSprite>(self);
            rsprite->rot = static_cast<float>(x);
            rsprite->dirty = true;
            return mrb::to_value(rsprite->rot, mrb);
        },
        MRB_ARGS_REQ(1));

    mrb_define_method(
        ruby, RSprite::rclass, "rotation",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rsprite = mrb::self_to<RSprite>(self);
            return mrb::to_value(rsprite->rot, mrb);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, RSprite::rclass, "move",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [x, y] = mrb::get_args<float, float>(mrb);
            auto* rspr = mrb::self_to<RSprite>(self);
            rspr->trans = {static_cast<float>(x), static_cast<float>(y)};
            rspr->dirty = true;
            return self;
        },
        MRB_ARGS_REQ(2));
}
