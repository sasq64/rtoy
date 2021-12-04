#include "rconsole.hpp"
#include "mrb/mrb_tools.hpp"
#include "rimage.hpp"

#include <coreutils/utf8.h>
#include <gl/program_cache.hpp>
#include <pix/pix.hpp>
#include <pix/pixel_console.hpp>

#include <mruby/array.h>

RConsole::RConsole(int w, int h, Style const& style)
    : RLayer{w, h},
      console(
          std::make_shared<PixConsole>(256, 256, style.font, style.font_size))
{
    default_fg = this->current_style.fg = gl::Color(style.fg).to_array();
    default_bg = this->current_style.bg = gl::Color(style.bg).to_array();
    reset();
}

RConsole::RConsole(
    int w, int h, Style const& style, std::shared_ptr<PixConsole> con)
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

void RConsole::update_pos(std::pair<int, int> const& cursor)
{
    xpos = cursor.first;
    ypos = cursor.second;
    // Calculate visible size.
    // TODO: Include tile_size? wrap attributes ?
    auto [char_width, char_height] = console->get_char_size();
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

void RConsole::text(std::string const& t, RStyle const* style)
{
    if (style == nullptr) { style = &current_style; }
    auto fg = gl::Color(style->fg).to_rgba();
    auto bg = gl::Color(style->bg).to_rgba();
    auto cursor = console->text(xpos, ypos, t, fg, bg);
    update_pos(cursor);
}

void RConsole::text(int x, int y, std::string const& t, RStyle const* style)
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

void RConsole::render(RLayer const* parent)
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

void RConsole::reg_class(mrb_state* ruby)
{
    RConsole::rclass = mrb::make_noinit_class<RConsole>(
        ruby, "Console", mrb::get_class<RLayer>(ruby));
    mrb::set_deleter<RConsole>(ruby, [](mrb_state* /*mrb*/, void* data) {});

    mrb::add_method<RConsole>(ruby, "print",
        [](RConsole* console, std::string const& text, RStyle* style) {
            if (style == nullptr) { style = &console->current_style; }
            console->text(text, style);
        });

    mrb::add_method<RConsole>(
        ruby, "clear", [](RConsole* console, RStyle* style) {
            if (style == nullptr) { style = &console->current_style; }
            console->current_style.fg = style->fg;
            console->current_style.bg = style->bg;
            auto fg = gl::Color(style->fg).to_rgba();
            auto bg = gl::Color(style->bg).to_rgba();
            console->console->fill(fg, bg);
            console->xpos = console->ypos = 0;
        });

    mrb::add_method<RConsole>(
        ruby, "fill", [](RConsole* console, RStyle* style) {
            if (style == nullptr) { style = &console->current_style; }
            auto fg = gl::Color(style->fg).to_rgba();
            auto bg = gl::Color(style->bg).to_rgba();
            console->console->fill(fg, bg);
        });

    mrb::add_method<RConsole>(ruby, "scroll",
        [](RConsole* console, int dy, int dx) { console->scroll(dy, dx); });

    mrb::add_method<RConsole>(ruby, "text",
        [](RConsole* console, int x, int y, std::string const& text,
            RStyle* style) {
            if (style == nullptr) { style = &console->current_style; }
            console->text(x, y, text, style);
        });

    mrb::add_method<&RConsole::get>(ruby, "get_tile");

    /* mrb_define_method( */
    /*     ruby, RConsole::rclass, "get_tile", */
    /*     [](mrb_state* mrb, mrb_value self) -> mrb_value { */
    /*         auto chr = mrb::method(&RConsole::get, mrb, self); */
    /*         return mrb::to_value(static_cast<int>(chr), mrb); */
    /*     }, */
    /*     MRB_ARGS_REQ(2)); */

    mrb::add_method<RConsole>(
        ruby, "get_char", [](RConsole* console, int x, int y) {
            auto chr = console->get(x, y);
            return utils::utf8_encode({chr, 0});
        });

    mrb::add_method<RConsole>(
        ruby, "set_tile_size", [](RConsole* console, int w, int h) {
            console->console->set_tile_size(w, h);
        });
    mrb::add_method<RConsole>(ruby, "get_tile_size", [](RConsole* console) {
        auto [tw, th] = console->console->get_char_size();
        return std::array<int, 2>{tw, th};
    });
    /* mrb_define_method( */
    /*     ruby, RConsole::rclass, "get_char", */
    /*     [](mrb_state* mrb, mrb_value self) -> mrb_value { */
    /*         auto chr = mrb::method(&RConsole::get, mrb, self); */
    /*         auto str = utils::utf8_encode({chr, 0}); */
    /*         return mrb::to_value(str, mrb); */
    /*     }, */
    /*     MRB_ARGS_REQ(2)); */

    /* mrb_define_method( */
    /*     ruby, RConsole::rclass, "set_tile_size", */
    /*     [](mrb_state* mrb, mrb_value self) -> mrb_value { */
    /*         auto* ptr = mrb::self_to<RConsole>(self); */
    /*         auto [x, y] = mrb::get_args<int, int>(mrb); */
    /*         ptr->console->set_tile_size(x, y); */
    /*         return mrb_nil_value(); */
    /*     }, */
    /*     MRB_ARGS_REQ(1)); */

    /* mrb_define_method( */
    /*     ruby, RConsole::rclass, "get_tile_size", */
    /*     [](mrb_state* mrb, mrb_value self) -> mrb_value { */
    /*         auto* ptr = mrb::self_to<RConsole>(self); */
    /*         auto [tw, th] = ptr->console->get_char_size(); */
    /*         std::array<int, 2> data{tw, th}; */
    /*         return mrb::to_value(data, mrb); */
    /*     }, */
    /*     MRB_ARGS_NONE()); */

    /* mrb_define_method( */
    /*     ruby, RConsole::rclass, "goto_xy", */
    /*     [](mrb_state* mrb, mrb_value self) -> mrb_value { */
    /*         auto* ptr = mrb::self_to<RConsole>(self); */
    /*         auto [x, y] = mrb::get_args<int, int>(mrb); */
    /*         ptr->xpos = x; */
    /*         ptr->ypos = y; */
    /*         // fmt::print("Goto {} {}\n", x, y); */
    /*         //  ptr->console->set_cursor(x, y); */
    /*         return mrb_nil_value(); */
    /*     }, */
    /*     MRB_ARGS_REQ(1)); */

    mrb::add_method<RConsole>(
        ruby, "goto_xy", [](RConsole* console, int x, int y) {
            console->xpos = x;
            console->ypos = y;
        });

    mrb::add_method<RConsole>(
        ruby, "put_char", [](RConsole* console, int c, int x, int y) {
            console->console->put_char(x, y, c);
        });

    /* mrb_define_method( */
    /*     ruby, RConsole::rclass, "put_char", */
    /*     [](mrb_state* mrb, mrb_value self) -> mrb_value { */
    /*         auto* ptr = mrb::self_to<RConsole>(self); */
    /*         auto [x, y, c] = mrb::get_args<int, int, int>(mrb); */
    /*         ptr->console->put_char(x, y, c); */
    /*         return mrb_nil_value(); */
    /*     }, */
    /*     MRB_ARGS_REQ(3)); */

    mrb::add_method<RConsole>(
        ruby, "clear_line", [](RConsole* console, int y, RStyle* style) {
            if (style == nullptr) { style = &console->current_style; }
            auto fg = gl::Color(style->fg).to_rgba();
            auto bg = gl::Color(style->bg).to_rgba();
            console->console->clear_area(0, y, -1, 1, fg, bg);
        });

    /* mrb_define_method( */
    /*     ruby, RConsole::rclass, "clear_line", */
    /*     [](mrb_state* mrb, mrb_value self) -> mrb_value { */
    /*         auto* ptr = mrb::self_to<RConsole>(self); */
    /*         auto n = mrb_get_argc(mrb); */
    /*         int y = 0; */
    /*         RStyle* style = &ptr->current_style; */
    /*         if (n == 1) { */
    /*             mrb_get_args(mrb, "i", &y); */
    /*         } else { */
    /*             mrb_get_args( */
    /*                 mrb, "id", &y, &style, mrb::get_data_type<RStyle>(mrb));
     */
    /*         } */
    /*         auto fg = gl::Color(style->fg).to_rgba(); */
    /*         auto bg = gl::Color(style->bg).to_rgba(); */
    /*         ptr->console->clear_area(0, y, -1, 1, fg, bg); */
    /*         return mrb_nil_value(); */
    /*     }, */
    /*     MRB_ARGS_REQ(1)); */

    mrb::add_method<RConsole>(ruby, "get_xy", [](RConsole* console) {
        return std::array<int, 2>{console->xpos, console->ypos};
    });

    /* mrb_define_method( */
    /*     ruby, RConsole::rclass, "get_xy", */
    /*     [](mrb_state* mrb, mrb_value self) -> mrb_value { */
    /*         auto* ptr = mrb::self_to<RConsole>(self); */
    /*         std::array values{ */
    /*             mrb::to_value(ptr->xpos, mrb), mrb::to_value(ptr->ypos,
     * mrb)}; */
    /*         return mrb_ary_new_from_values(mrb, 2, values.data()); */
    /*     }, */
    /*     MRB_ARGS_REQ(3)); */

    mrb::add_method<RConsole>(ruby, "set_tile_image",
        [](RConsole* console, int index, RImage* image) {
            console->console->set_tile_image(index, image->texture);
        });

    /* mrb_define_method( */
    /*     ruby, RConsole::rclass, "set_tile_image", */
    /*     [](mrb_state* mrb, mrb_value self) -> mrb_value { */
    /*         uint32_t index = 0; */
    /*         RImage* image = nullptr; */
    /*         mrb_get_args( */
    /*             mrb, "id", &index, &image, mrb::get_data_type<RImage>(mrb)); */
    /*         auto* rconsole = mrb::self_to<RConsole>(self); */
    /*         rconsole->console->set_tile_image(index, image->texture); */
    /*         return mrb_nil_value(); */
    /*     }, */
    /*     MRB_ARGS_REQ(2)); */
}

void RConsole::reset()
{
    RLayer::reset();
    console->reset();

    auto [_, tile_height] = console->get_char_size();

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
