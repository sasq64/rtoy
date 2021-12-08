
#include "rimage.hpp"

#include <gl/program_cache.hpp>
#include <mrb/mrb_tools.hpp>

#include <array>
#include <memory>

//#include <jpeg_decoder.h>

void RImage::reg_class(mrb_state* ruby)
{
    mrb::make_noinit_class<RImage>(ruby, "Image");

    mrb::add_class_method<RImage>(
        ruby, "from_file", [](std::string const& name) {
            auto img = pix::load_png(name);
            if (img.ptr == nullptr) { return static_cast<RImage*>(nullptr); }
            return new RImage(img);
        });

    // TODO: Fix const& for array args
    mrb::add_class_method<RImage>(
        ruby, "solid", [](int w, int h, std::array<float, 4> color) {
            auto col32 = gl::Color(color).to_rgba();
            auto texref = gl_wrap::TexRef(w, h);
            texref.tex->fill(col32);
            return new RImage(texref);
        });

    mrb::add_method<&RImage::width>(ruby, "width");
    mrb::add_method<&RImage::height>(ruby, "height");

    mrb::add_method<RImage>(ruby, "split", [](RImage* self, int w, int h) {
        float u0 = self->texture.uvs[0];
        float v0 = self->texture.uvs[1];
        float u1 = self->texture.uvs[4];
        float v1 = self->texture.uvs[5];
        float du = (u1 - u0) / static_cast<float>(w);
        float dv = (v1 - v0) / static_cast<float>(h);

        std::vector<RImage*> images;

        float u = u0;
        float v = v0;
        int x = 0;
        int y = 0;
        while (true) {
            if (x == w) {
                u = u0;
                v += dv;
                x = 0;
                y++;
            }
            if (y == h) { break; }
            auto* rimage = new RImage(self->texture);
            rimage->texture.tex = self->texture.tex;
            rimage->texture.uvs = {u, v, u + du, v, u + du, v + dv, u, v + dv};
            images.push_back(rimage); // mrb::new_data_obj(mrb, rimage));
            u += du;
            x++;
        }
        return images;
    });
}

RImage::RImage(pix::Image const& image)
{
    if (texture.tex == nullptr) {
        texture.tex = std::make_shared<gl::Texture>(
            image.width, image.height, image.ptr, GL_RGBA, image.format);
    }
}

void RImage::draw(double x, double y, double scale)
{
    // fmt::print("Draw {}x{} at {},{}\n", img_width, img_height, x, y);
    // upload();
    texture.bind();
    gl_wrap::ProgramCache::get_instance().textured.use();
    pix::draw_quad_uvs(x, y, width() * scale, height() * scale, texture.uvs);
}
