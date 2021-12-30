
#include "rimage.hpp"

#include <gl/program_cache.hpp>
#include <mrb/mrb_tools.hpp>

#include <array>
#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

void RImage::reg_class(mrb_state* ruby)
{
    mrb::make_noinit_class<RImage>(ruby, "Image");

    mrb::add_class_method<RImage>(
        ruby, "from_file", [](std::string const& name) {
            fs::path p{name};
            auto ext = p.extension();
            auto img = ext == ".png" ? pix::load_png(name) : pix::load_jpg(p);
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

    // std::array vertexData{
    //     x0, y0, x1, y0, x1, y1, x0, y1, 0.F, 0.F, 1.F, 0.F, 1.F, 1.F,
    //     0.F, 1.F};
    mrb::add_method<RImage>(ruby, "trim",
        [](RImage* self, int left, int top, int right, int bottom) {
            auto w = self->tex_ref.tex->width;
            auto h = self->tex_ref.tex->height;
            auto lf = static_cast<double>(left) / w;
            auto tf = static_cast<double>(top) / h;
            auto rf = static_cast<double>(right) / w;
            auto bf = static_cast<double>(bottom) / h;
            auto& uvs = self->tex_ref.uvs;
            uvs[0] += static_cast<float>(lf);
            uvs[1] += static_cast<float>(tf);
            uvs[2] -= static_cast<float>(rf);
            uvs[3] += static_cast<float>(tf);
            uvs[4] -= static_cast<float>(rf);
            uvs[5] -= static_cast<float>(bf);
            uvs[6] += static_cast<float>(lf);
            uvs[7] -= static_cast<float>(bf);
        });

    mrb::add_method<RImage>(ruby, "split", [](RImage* self, int w, int h) {
        float u0 = self->tex_ref.uvs[0];
        float v0 = self->tex_ref.uvs[1];
        float u1 = self->tex_ref.uvs[4];
        float v1 = self->tex_ref.uvs[5];
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
            auto* rimage = new RImage(self->tex_ref);
            rimage->tex_ref.tex = self->tex_ref.tex;
            rimage->tex_ref.uvs = {u, v, u + du, v, u + du, v + dv, u, v + dv};
            images.push_back(rimage); // mrb::new_data_obj(mrb, rimage));
            u += du;
            x++;
        }
        return images;
    });
}

RImage::RImage(pix::Image const& image)
{
    tex_ref.tex = std::make_shared<gl::Texture>(
        image.width, image.height, image.ptr, GL_RGBA, image.format);
}

void RImage::draw(double x, double y, double scale)
{
    fmt::print("Draw {} at {}x{}\n", tex_ref.tex->tex_id, x, y);
    // upload();
    tex_ref.bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    pix::set_transform();
    gl_wrap::ProgramCache::get_instance().textured.use();
    pix::draw_quad_uvs(x, y, width() * scale, height() * scale, tex_ref.uvs);
}
