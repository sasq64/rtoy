
#include "rimage.hpp"

#include "rlayer.hpp"

#include "mrb_tools.hpp"

#include <pix/gl_console.hpp>

#include <mruby.h>
#include <mruby/class.h>

#include <array>
#include <memory>
#include "mrb_tools.hpp"

#include <jpeg_decoder.h>

mrb_data_type RImage::dt{"Image", [](mrb_state*, void* data) {
                             //fmt::print("Deleting image\n");
                             //delete (RImage*)data;
                         }};

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
            auto* image = new RImage(img);
            return mrb::new_data_obj(mrb, image);
        },
        MRB_ARGS_REQ(1));

    mrb_define_method(
        ruby, RImage::rclass, "width",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            return mrb::to_value(mrb::self_to<RImage>(self)->image.width, mrb);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, RImage::rclass, "height",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            return mrb::to_value(mrb::self_to<RImage>(self)->image.height, mrb);
        },
        MRB_ARGS_NONE());
}

void RImage::draw(float x, float y)
{
    fmt::print("Draw {}x{} at {},{}\n", image.width, image.height, x, y);
    if (texture.tex_id == 0) {
        texture = gl::Texture(
            image.width, image.height, image.ptr, GL_RGBA, image.format);
    }
    texture.bind();
    pix::draw_quad({x, y}, {image.width, image.height});
}
