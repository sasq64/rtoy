#include "rfont.hpp"

#include "mrb/mrb_tools.hpp"
#include "rimage.hpp"

RFont::RFont(std::string const& fname) : font(fname.c_str()) {}

RImage* RFont::render(std::string const& txt, uint32_t color, int n)
{
    font.set_pixel_size(n);
    auto [w, h] = font.text_size(txt);
    pix::Image img(w, h);
    font.render_text(txt, reinterpret_cast<uint32_t*>(img.ptr), color,
        img.width, img.width, img.height);
    return new RImage(img);
}

void RFont::reg_class(mrb_state* ruby)
{
    mrb::make_noinit_class<RFont>(ruby, "Font");

    mrb::add_class_method<RFont>(ruby, "from_file", [](std::string const& name) {
        return new RFont(name);
    });

    mrb::add_method<RFont>(ruby, "get_size", [](RFont* self, std::string const& txt, int n) {
        self->font.set_pixel_size(n);
        auto [w, h] = self->font.text_size(txt);
        return std::array{w, h};
    });

    mrb::add_method<RFont>(ruby, "render", [](RFont* self, mrb::ArgN n, std::string const& txt, int size, std::array<float, 4> color) {
        auto col_u = n > 2 ? gl::Color(color).to_bgra() : 0xffffff00;
        fmt::print("Text {} color {:x}\n", txt, col_u);
        return self->render(txt, col_u, size);
    });
}

