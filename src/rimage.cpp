
#include "rimage.hpp"

#include "mruby/value.h"

#include "mrb_tools.hpp"

#include <gl/program_cache.hpp>

#include <mruby.h>
#include <mruby/class.h>

#include "mrb_tools.hpp"
#include <array>
#include <memory>

//#include <jpeg_decoder.h>

mrb_data_type RImage::dt{
    "Image", [](mrb_state*, void* data) { delete static_cast<RImage*>(data); }};

void RImage::reg_class(mrb_state* ruby)
{
    RImage::rclass = mrb_define_class(ruby, "Image", ruby->object_class);
    MRB_SET_INSTANCE_TT(RImage::rclass, MRB_TT_DATA);

    mrb_define_class_method(
        ruby, RImage::rclass, "from_file",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            const char* name{};
            mrb_get_args(mrb, "z", &name);
            auto img = pix::load_png(name);
            if (img.ptr == nullptr) { return mrb_nil_value(); }
            auto* rimage = new RImage(img);
            return mrb::new_data_obj(mrb, rimage);
        },
        MRB_ARGS_REQ(1));

    mrb_define_class_method(
        ruby, RImage::rclass, "solid",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            mrb_value color;
            int w{};
            int h{};
            mrb_get_args(mrb, "iio", &w, &h, &color);
            auto col32 =
                gl::Color(mrb::to_array<float, 4>(color, mrb)).to_rgba();
            auto texref = gl_wrap::TexRef(w, h);
            texref.tex->fill(col32);
            auto* rimage = new RImage(texref);
            return mrb::new_data_obj(mrb, rimage);
        },
        MRB_ARGS_REQ(3));

    mrb_define_method(
        ruby, RImage::rclass, "save",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            const char* name{};
            mrb_get_args(mrb, "z", &name);
            auto* thiz = mrb::self_to<RImage>(self);
            auto bytes = thiz->texture.read_pixels();
            pix::Image img{thiz->width(), thiz->height(), bytes.data()};
            pix::save_png(img, name);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    mrb_define_method(
        ruby, RImage::rclass, "width",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            return mrb::to_value(mrb::self_to<RImage>(self)->width(), mrb);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, RImage::rclass, "height",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            return mrb::to_value(mrb::self_to<RImage>(self)->height(), mrb);
        },
        MRB_ARGS_NONE());
    mrb_define_method(
        ruby, RImage::rclass, "split",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [w, h] = mrb::get_args<int, int>(mrb);
            auto* thiz = mrb::self_to<RImage>(self);

            float u0 = thiz->texture.uvs[0];
            float v0 = thiz->texture.uvs[1];
            float u1 = thiz->texture.uvs[4];
            float v1 = thiz->texture.uvs[5];
            auto du = (u1 - u0) / static_cast<float>(w);
            auto dv = (v1 - v0) / static_cast<float>(h);

            std::vector<mrb_value> images;

            float u = u0;
            float v = v0;
            // fmt::print("{} -> {}\n", u0, u1);
            int x = 0;
            int y = 0;
            while (true) {
                // fmt::print("{} + {} =  {}\n", u, du, u + du);
                if (x == w) {
                    u = u0;
                    v += dv;
                    x = 0;
                    y++;
                }
                if (y == h) { break; }
                auto* rimage = new RImage(thiz->texture);
                rimage->texture.tex = thiz->texture.tex;
                rimage->texture.uvs = {
                    u, v, u + du, v, u + du, v + dv, u, v + dv};
                images.push_back(mrb::new_data_obj(mrb, rimage));
                u += du;
                x++;
            }

            return mrb::to_value(images, mrb);
        },
        MRB_ARGS_REQ(2));
}

void RImage::upload(pix::Image const& image)
{
    if (texture.tex == nullptr) {
        texture.tex = std::make_shared<gl::Texture>(
            image.width, image.height, image.ptr, GL_RGBA, image.format);
    }
}

void RImage::crop() {}

void RImage::draw(double x, double y, double scale)
{
    // fmt::print("Draw {}x{} at {},{}\n", img_width, img_height, x, y);
    // upload();
    texture.bind();
    gl_wrap::ProgramCache::get_instance().textured.use();
    pix::draw_quad_uvs(x, y, width() * scale, height() * scale, texture.uvs);
}
