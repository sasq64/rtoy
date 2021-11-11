
#include "rsprites.hpp"
#include "error.hpp"
#include "gl/buffer.hpp"
#include "mrb_tools.hpp"
#include "mruby/value.h"
#include "rimage.hpp"

#include <gl/gl.hpp>
#include <gl/program_cache.hpp>
#include <pix/pix.hpp>

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <mruby/class.h>

#include <algorithm>

// language=glsl
static std::string vertex_shader{R"(
    #ifdef GL_ES
        precision mediump float;
    #endif
        attribute vec2 in_pos;
        uniform mat4 layer_transform;
        uniform mat4 in_transform;
        attribute vec2 in_uv;
        varying vec2 out_uv;
        void main() {
            vec4 v = layer_transform * in_transform * vec4(in_pos, 0, 1);
            gl_Position = vec4( v.x, v.y, 0, 1 );
            out_uv = in_uv;
    })"};

// language=glsl
static std::string fragment_shader{R"(
    #ifdef GL_ES
        precision mediump float;
    #endif
        uniform vec4 in_color;
        uniform sampler2D in_tex;
        varying vec2 out_uv;
        void main() {
            gl_FragColor = texture2D(in_tex, out_uv) * in_color;
        })"};

mrb_data_type RSprite::dt{"Sprite", [](mrb_state*, void* data) {
                              auto* sprite = static_cast<RSprite*>(data);
                              // fmt::print("free {} {}\n", data, sprite->held);
                              if (sprite->held) {
                                  sprite->held = false;
                              } else {
                                  delete sprite;
                              }
                          }};

mrb_data_type RSprites::dt{"Sprites", [](mrb_state*, void* data) {}};

void RSprite::update_tx(double screen_width, double screen_height)
{
    glm::mat4x4 m(1.0F);
    m = glm::scale(m, glm::vec3(2.0 / screen_width, 2.0 / screen_height, 1.0));
    m = glm::translate(m, glm::vec3(-screen_width / 2.0 + trans[0],
                              screen_height / 2.0 - trans[1], 0));
    m = glm::translate(m, glm::vec3(texture.width() / 2.0 * scale[0],
                              -texture.height() / 2.0 * scale[1], 0));
    m = glm::rotate(m, -rot, glm::vec3(0.0, 0.0, 1.0));

    m = glm::scale(
        m, glm::vec3(static_cast<float>(texture.width()) * scale[0] / 2,
               static_cast<float>(texture.height()) * scale[1] / -2, 1.0));
    memcpy(transform.data(), glm::value_ptr(m), sizeof(float) * 16);
}

RSprites::RSprites(mrb_state* _ruby, int w, int h) : RLayer{w, h}, ruby{_ruby}
{
    program = gl_wrap::Program(gl_wrap::VertexShader{vertex_shader},
        gl_wrap::FragmentShader{fragment_shader});
    tx_location = glGetUniformLocation(program.program, "in_transform");
    col_location = glGetUniformLocation(program.program, "in_color");
    pos = program.getAttribute("in_pos");
    uv = program.getAttribute("in_uv");
}

void RSprites::purge()
{
    for (auto& [_, batch] : batches) {
        for (auto& sprite : batch.sprites) {
            // fmt::print("purge {} {}\n", (void*)sprite, sprite->held);
            if (sprite->held) {
                sprite->held = false;
            } else {
                delete sprite;
            }
        }
    }
    batches.clear();
}

void RSprites::reset()
{
    RLayer::reset();
    purge();
}

void RSprites::clear()
{
    purge();
}

void RSprites::collide()
{
    for (auto& group : groups) {
        auto& group0 = colliders[group.from];
        auto& group1 = colliders[group.to];
        auto it = group0.begin();
        while (it != group0.end()) {
            auto* sprite1 = *it;
            if (sprite1->texture.tex == nullptr) {
                ++it;
                continue;
            }
            auto* coll1 = sprite1->collider;
            auto s1 = sprite1->scale[0];
            auto x1 = sprite1->trans[0] + coll1->width * s1 / 2;
            auto y1 = sprite1->trans[1] + coll1->height * s1 / 2;
            auto it2 = group1.begin();
            while (it2 != group1.end()) {
                auto* sprite2 = *it2;
                if (sprite2->texture.tex == nullptr) {
                    ++it2;
                    continue;
                }
                auto* coll2 = sprite2->collider;
                auto s2 = sprite2->scale[0];
                auto x = (sprite2->trans[0] + coll2->width * s2 / 2) - x1;
                auto y = (sprite2->trans[1] + coll2->height * s2 / 2) - y1;
                auto d2 = x * x + y * y;
                auto r = (coll1->radius * s1 + coll2->radius * s2);
                if (d2 < r * r) {
                    if (group.handler) {
                        call_proc(ruby, group.handler, sprite1->value,
                            sprite2->value);
                    }
                }
                ++it2;
            }
            ++it;
        }
    }
}

bool RSprites::draw_batch(SpriteBatch& batch)
{
    float last_alpha = -1;
    batch.texture->bind();
    auto it = batch.sprites.begin();
    while (it != batch.sprites.end()) {
        auto* sprite = *it;

        int count = 0;
        if (sprite->texture.tex == nullptr) {
            it = batch.sprites.erase(it);
            // fmt::print("erase {} {}\n", (void*)sprite, sprite->held);
            if (sprite->held) {
                sprite->held = false;
            } else {
                delete sprite;
            }
            continue;
        }
        // When we render sprites, we sort them into `colliders` according
        // to their group
        if (sprite->collider != nullptr) {
            colliders[sprite->collider->group].push_back(sprite);
        }
        //if (last_alpha != sprite->alpha) {
            //gl::Color fg = current_style.fg;
            //fg.alpha = sprite->alpha;
            program.setUniform(col_location, sprite->color);
            //last_alpha = sprite->alpha;
        //}
        if (sprite->dirty) {
            sprite->dirty = false;
            sprite->update_tx(width, height);
        }
        program.setUniform(tx_location, sprite->transform);
        sprite->vbo.bind();
        gl::vertexAttrib(pos, 2, gl::Type::Float, 0 * sizeof(GLfloat), 0);
        gl::vertexAttrib(uv, 2, gl::Type::Float, 0 * sizeof(GLfloat), 8 * 4);
        gl::drawArrays(gl::Primitive::TriangleFan, 0, 4);
        it++;
    }
    return batch.sprites.empty();
}

void RSprites::render(RLayer const* parent)
{
    if (!enabled) { return; }

    update_tx(parent);
    glEnable(GL_BLEND);
    program.use();
    program.setUniform("layer_transform", transform);
    pos.enable();
    uv.enable();

    for (auto& [_, c] : colliders) {
        c.clear();
    }

    auto it = batches.begin();
    while (it != batches.end()) {
        if (it->second.sprites.empty()) {
            // fmt::print("Batch erased\n");
            it = batches.erase(it);
            if (batches.empty()) { fmt::print("No more batches\n"); }
            continue;
        }
        draw_batch(it->second);
        it++;
    }

    collide();

    if (fixed_batch.texture != nullptr) { draw_batch(fixed_batch); }
    pos.disable();
    uv.disable();
}

void RSprite::update_collision() const
{
    collider->width = static_cast<float>(texture.width());
    collider->height = static_cast<float>(texture.height());
    auto r = collider->width;
    if (collider->height > collider->width) { r = collider->height; }
    collider->radius = r / 2;
}

RSprite* RSprites::add_particle(int size, uint32_t color)
{
    auto& batch = particle_batch;
    std::array vertexData{-1.F, -1.F, 1.F, -1.F, 1.F, 1.F, -1.F, 1.F};
    auto* sprite =
        new RSprite{gl_wrap::ArrayBuffer<GL_STATIC_DRAW>{vertexData}};
    sprite->update_tx(width, height);
    batch.sprites.push_back(sprite);
    return sprite;
}

RSprite* RSprites::add_sprite(RImage* image, int flags)
{
    auto& batch =
        flags == 1 ? fixed_batch : batches[image->texture.tex->tex_id];
    if (batch.texture == nullptr) {
        batch.texture = image->texture.tex;
        // batch.image = image->image;
    }
    auto& uvs = image->texture.uvs;
    std::array vertexData{-1.F, -1.F, 1.F, -1.F, 1.F, 1.F, -1.F, 1.F, 0.F, 0.F,
        1.F, 0.F, 1.F, 1.F, 0.F, 1.F};
    std::copy(uvs.begin(), uvs.end(), vertexData.begin() + 8);

    auto* sprite =
        new RSprite{gl_wrap::ArrayBuffer<GL_STATIC_DRAW>{vertexData}};
    sprite->texture = image->texture;
    sprite->trans[0] = static_cast<float>(sprite->texture.x());
    sprite->trans[1] = static_cast<float>(sprite->texture.y());
    sprite->update_tx(width, height);
    batch.sprites.push_back(sprite);
    // fmt::print("Add sprite {} {}\n", (void*)sprite, sprite->held);

    return sprite;
}

void RSprites::remove_sprite(RSprite* spr)
{
    // TODO: Optimize
    spr->texture.tex = nullptr;
}

void RSprites::reg_class(mrb_state* ruby)
{
    RSprites::rclass = mrb_define_class(ruby, "Sprites", RLayer::rclass);
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
            spr->value = mrb::new_data_obj(mrb, spr);
            return spr->value;
        },
        MRB_ARGS_REQ(3));

    mrb_define_method(
        ruby, rclass, "on_collision",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* sprites = mrb::self_to<RSprites>(self);
            mrb_value blk;
            mrb_value s0;
            mrb_value s1;
            mrb_get_args(mrb, "oo&", &s0, &s1, &blk);
            auto g0 = mrb_obj_to_sym(mrb, s0);
            auto g1 = mrb_obj_to_sym(mrb, s1);

            CollisionGroup* group = nullptr;
            for (auto& g : sprites->groups) {
                if (g.from == g0 && g.to == g1) { group = &g; }
            }
            if (group == nullptr) { group = &sprites->groups.emplace_back(); }
            group->from = g0;
            group->to = g1;
            if (!mrb_nil_p(blk)) { group->handler = mrb::RubyPtr{mrb, blk}; }
            return mrb_nil_value();
        },
        MRB_ARGS_BLOCK() | MRB_ARGS_REQ(1));

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
        ruby, RSprite::rclass, "collider=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            mrb_value sym;
            mrb_get_args(mrb, "o", &sym);
            auto id = mrb_obj_to_sym(mrb, sym);
            auto* rspr = mrb::self_to<RSprite>(self);
            if (rspr->collider == nullptr) {
                rspr->collider = new Collider();
                rspr->update_collision();
            }
            rspr->collider->group = id;
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

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
        ruby, RSprite::rclass, "width",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rspr = mrb::self_to<RSprite>(self);
            return mrb::to_value(rspr->texture.width(), mrb);
        },
        MRB_ARGS_NONE());
    mrb_define_method(
        ruby, RSprite::rclass, "height",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rspr = mrb::self_to<RSprite>(self);
            return mrb::to_value(rspr->texture.height(), mrb);
        },
        MRB_ARGS_NONE());
    mrb_define_method(
        ruby, RSprite::rclass, "size",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rspr = mrb::self_to<RSprite>(self);
            return mrb::to_value(
                std::array<float, 2>{static_cast<float>(rspr->texture.width()),
                    static_cast<float>(rspr->texture.height())},
                mrb);
        },
        MRB_ARGS_NONE());
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
            auto* rimage = new RImage(rspr->texture);
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
            rspr->color[3] = x;
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));
    mrb_define_method(
        ruby, RSprite::rclass, "alpha",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rspr = mrb::self_to<RSprite>(self);
            return mrb::to_value(rspr->color[3], mrb);
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
        ruby, RSprite::rclass, "pos=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rspr = mrb::self_to<RSprite>(self);
            auto [av] = mrb::get_args<mrb_value>(mrb);
            rspr->trans = mrb::to_array<float, 2>(av, mrb);
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

    mrb_define_method(
        ruby, RSprite::rclass, "color=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
          auto [av] = mrb::get_args<mrb_value>(mrb);
          auto* rsprite = mrb::self_to<RSprite>(self);
          rsprite->color = mrb::to_array<float, 4>(av, mrb);
          return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    mrb_define_method(
        ruby, RSprite::rclass, "color",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
          auto* rsprite = mrb::self_to<RSprite>(self);
          return mrb::to_value(rsprite->color, mrb);
        },
        MRB_ARGS_NONE());
}
