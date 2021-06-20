
#include "rimage.hpp"

#include "mruby/value.h"
#include "rlayer.hpp"

#include "mrb_tools.hpp"

#include <pix/gl_console.hpp>

#include <mruby.h>
#include <mruby/class.h>

#include "mrb_tools.hpp"
#include <array>
#include <memory>

#include <gif_load.h>
#include <jpeg_decoder.h>

#include <filesystem>

namespace fs = std::filesystem;

pix::Image load_gif(std::string const& name)
{

    auto sz = fs::file_size(name);
    std::vector<uint8_t> data(sz);
    FILE* fp = fopen(name.c_str(), "rbe");
    fread(data.data(), 1, sz, fp);
    fclose(fp);

    GIF_Load(
        data.data(), static_cast<long>(data.size()),
        [](void* data, struct GIF_WHDR* h) { 
            fmt::print("Found frame {}x{} at {},{} ({}x{}) {} colors\n",
            h->frxd, h->fryd, h->frxo, h->fryo, h->xdim, h->ydim, h->clrs);
            // TODO
            // Create w*y RGBA image, copy pixels
            // Needs to return array of images or take a block
            //
        },
        nullptr, nullptr, 0L);
    return pix::Image{};
}

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
            fs::path p = name;
            pix::Image img;
            if (p.extension() == ".gif") {
                img = load_gif(name);
            } else {
                img = pix::load_png(name);
            }
            if (img.ptr == nullptr) { return mrb_nil_value(); }
            auto* rimage = new RImage(img);
            return mrb::new_data_obj(mrb, rimage);
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
    mrb_define_method(
        ruby, RImage::rclass, "split",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [w, h] = mrb::get_args<int, int>(mrb);
            auto* thiz = mrb::self_to<RImage>(self);
            thiz->upload();

            float u0 = thiz->texture.uvs[0];
            float v0 = thiz->texture.uvs[1];
            float u1 = thiz->texture.uvs[4];
            float v1 = thiz->texture.uvs[5];
            auto du = (u1 - u0) / static_cast<float>(w);
            auto dv = (v1 - v0) / static_cast<float>(h);

            std::vector<mrb_value> images;

            float u = u0;
            float v = v0;
            while (true) {
                if (u + du > u1) {
                    u = u0;
                    v += dv;
                }
                if (v + dv > v1) { break; }
                auto* rimage = new RImage(thiz->image);
                rimage->texture.tex = thiz->texture.tex;
                rimage->texture.uvs = {
                    u, v, u + du, v, u + du, v + dv, u, v + dv};
                images.push_back(mrb::new_data_obj(mrb, rimage));
                u += du;
            }

            return mrb::to_value(images, mrb);
        },
        MRB_ARGS_REQ(2));
}

void RImage::upload()
{
    if (texture.tex == nullptr) {
        texture.tex = std::make_shared<gl::Texture>(
            image.width, image.height, image.ptr, GL_RGBA, image.format);
    }
}

void RImage::draw(float x, float y)
{
    fmt::print("Draw {}x{} at {},{}\n", image.width, image.height, x, y);
    upload();
    texture.bind();
    pix::draw_quad_uvs(x, y, width(), height(), texture.uvs);
}
