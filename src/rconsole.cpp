#include "rconsole.hpp"
#include "mrb/mrb_tools.hpp"
#include "rimage.hpp"

#include <coreutils/utf8.h>
#include <gl/program_cache.hpp>
#include <pix/pix.hpp>
#include <pix/pixel_console.hpp>

#include <mruby/array.h>

RConsole::RConsole(int w, int h, Style const &style)
    : RLayer{w, h},
      console(
          std::make_shared<PixConsole>(256, 256, style.font, style.font_size))
{
    default_fg = this->current_style.fg = gl::Color(style.fg).to_array();
    default_bg = this->current_style.bg = gl::Color(style.bg).to_array();
    reset();
}

RConsole::RConsole(
    int w, int h, Style const &style, std::shared_ptr<PixConsole> con)
    : RLayer{w, h}, console{con}
{
    default_fg = this->current_style.fg = gl::Color(style.fg).to_array();
    default_bg = this->current_style.bg = gl::Color(style.bg).to_array();
    reset();
}

void RConsole::clear()
{
    auto fg = gl::Color(current_style.fg).to_rgba();
    auto bg = gl::Color(current_style.bg).to_rgba();
    console->fill(fg, bg);
    xpos = ypos = 0;
}

void RConsole::update_pos(std::pair<int, int> const &cursor)
{
    xpos = cursor.first;
    ypos = cursor.second;
    // Calculate visible size.
    // TODO: Include tile_size? wrap attributes ?
    auto[char_width, char_height] = console->get_char_size();
    int w = static_cast<int>(
        static_cast<float>(width) / static_cast<float>(char_width) / scale[0]);
    int h = static_cast<int>(static_cast<float>(height) /
        static_cast<float>(char_height) / scale[1]);
    while (xpos >= w) {
        xpos -= w;
        ypos++;
    }
    while (ypos >= h) {
        console->scroll(-1, 0);
        ypos--;
    }
}

void RConsole::text(std::string const &t, RStyle const *style)
{
    if (style == nullptr) { style = &current_style; }
    auto fg = gl::Color(style->fg).to_rgba();
    auto bg = gl::Color(style->bg).to_rgba();
    auto cursor = console->text(xpos, ypos, t, fg, bg);
    update_pos(cursor);
}

void RConsole::text(int x, int y, std::string const &t, RStyle const *style)
{
    if (style == nullptr) { style = &current_style; }
    auto fg = gl::Color(style->fg).to_rgba();
    auto bg = gl::Color(style->bg).to_rgba();
    console->text(x, y, t, fg, bg);
}

void RConsole::fill(uint32_t fg, uint32_t bg)
{
    return console->fill(fg, bg);
}

void RConsole::scroll(int dy, int dx)
{
    console->scroll(dy, dx);
    console->flush();
}

void RConsole::render(RLayer const *parent)
{
    if (!enabled) { return; }
    auto s =
        std::pair{scale[0] * parent->scale[0], scale[1] * parent->scale[1]};
    auto o =
        std::pair{trans[0] + parent->trans[0], trans[1] + parent->trans[1]};
    console->flush();
    update_tx(parent);
    console->render(o, s);
}

uint32_t RConsole::get(int x, int y) const
{
    return console->get_char(x, y);
}

namespace mrb
{
template<typename T>
struct classdef
{
    mrb_state *ruby;
    classdef(mrb_state *mrb)
        : ruby{mrb}
    {}
    template<typename FN>
    void method(std::string const &name, FN &&fn)
    {
        mrb::add_method<T>(ruby, name, std::forward<FN>(fn));
    }
};
};

void RConsole::reg_class(mrb_state *ruby)
{
    RConsole::rclass = mrb::make_noinit_class<RConsole>(
        ruby, "Console", mrb::get_class<RLayer>(ruby));
    mrb::set_deleter<RConsole>(ruby, [](mrb_state *, void *)
    {});

    mrb::classdef<RConsole> c{ruby};

    c.method("print", [](RConsole *self, std::string const &text, RStyle *style)
    {
        if (style == nullptr) { style = &self->current_style; }
        self->text(text, style);
    });

    mrb::add_method<RConsole>(
        ruby, "clear", [](RConsole *self, RStyle *style)
        {
            if (style == nullptr) { style = &self->current_style; }
            self->current_style.fg = style->fg;
            self->current_style.bg = style->bg;
            auto fg = gl::Color(style->fg).to_rgba();
            auto bg = gl::Color(style->bg).to_rgba();
            self->console->fill(fg, bg);
            self->xpos = self->ypos = 0;
        });

    mrb::add_method<RConsole>(
        ruby, "fill", [](RConsole *self, RStyle *style)
        {
            if (style == nullptr) { style = &self->current_style; }
            auto fg = gl::Color(style->fg).to_rgba();
            auto bg = gl::Color(style->bg).to_rgba();
            self->console->fill(fg, bg);
        });

    mrb::add_method<RConsole>(ruby, "scroll",
                              [](RConsole *self, int dy, int dx)
                              { self->scroll(dy, dx); });

    c.method("text",
             [](RConsole *self, int x, int y, std::string const &text,
                RStyle *style)
             {
                 if (style == nullptr) { style = &self->current_style; }
                 self->text(x, y, text, style);
             });

    mrb::add_method<&RConsole::get>(ruby, "get_tile");

    mrb::add_method<RConsole>(
        ruby, "get_char", [](RConsole *self, int x, int y)
        {
            auto chr = self->get(x, y);
            return utils::utf8_encode({chr, 0});
        });

    mrb::add_method<RConsole>(
        ruby, "set_tile_size", [](RConsole *self, int w, int h)
        {
            self->console->set_tile_size(w, h);
        });
    mrb::add_method<RConsole>(ruby, "get_tile_size", [](RConsole *self)
    {
        auto[tw, th] = self->console->get_char_size();
        return std::array<int, 2>{tw, th};
    });

    mrb::add_method<RConsole>(
        ruby, "goto_xy", [](RConsole *self, int x, int y)
        {
            self->xpos = x;
            self->ypos = y;
        });

    mrb::add_method<RConsole>(
        ruby, "put_char", [](RConsole *self, int c, int x, int y)
        {
            self->console->put_char(x, y, c);
        });

    mrb::add_method<RConsole>(
        ruby, "clear_line", [](RConsole *self, int y, RStyle *style)
        {
            if (style == nullptr) { style = &self->current_style; }
            auto fg = gl::Color(style->fg).to_rgba();
            auto bg = gl::Color(style->bg).to_rgba();
            self->console->clear_area(0, y, -1, 1, fg, bg);
        });

    mrb::add_method<RConsole>(ruby, "get_xy", [](RConsole *self)
    {
        return std::array<int, 2>{self->xpos, self->ypos};
    });

    mrb::add_method<RConsole>(ruby, "set_tile_image",
                              [](RConsole *self, int index, RImage *image)
                              {
                                  self->console->set_tile_image(index, image->texture);
                              });
}

void RConsole::reset()
{
    RLayer::reset();
    console->reset();

    auto[_, tile_height] = console->get_char_size();

    int lines = height / tile_height;
    float s = 1.0F;
    int total = lines;
    while (total > 50) {
        s += 1.0F;
        total -= lines;
    }

    scale = {s, s};

    current_style.fg = default_fg;
    current_style.bg = default_bg;

    clear();
}
