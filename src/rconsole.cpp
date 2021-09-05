#include "rconsole.hpp"
#include "mrb_tools.hpp"
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
    /* trans = {0.0F, 0.0F}; */
    /* scale = {2.0F, 2.0F}; */

    /* rot = 0.0F; */
    /* update_tx(); */
}

void RConsole::clear()
{
    auto fg = gl::Color(current_style.fg).to_rgba();
    auto bg = gl::Color(current_style.bg).to_rgba();
    console->fill(fg, bg);
    // console->flush();
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

void RConsole::render()
{
    if (!enabled) { return; }
    console->flush();
    console->render();
}

uint32_t RConsole::get(int x, int y) const
{
    return console->get_char(x, y);
}

void RConsole::reg_class(mrb_state* ruby)
{
    rclass = mrb_define_class(ruby, "Console", RLayer::rclass);

    MRB_SET_INSTANCE_TT(RConsole::rclass, MRB_TT_DATA);

    mrb_define_method(
        ruby, RConsole::rclass, "print",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* ptr = mrb::self_to<RConsole>(self);
            ptr->text(std::get<0>(mrb::get_args<std::string>(mrb)));
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    mrb_define_method(
        ruby, RConsole::rclass, "clear",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* ptr = mrb::self_to<RConsole>(self);
            auto n = mrb_get_argc(mrb);
            RStyle* style = &ptr->current_style;
            if (n == 1) { mrb_get_args(mrb, "d", &style, &RStyle::dt); }
            ptr->current_style.fg = style->fg;
            ptr->current_style.bg = style->bg;
            auto fg = gl::Color(style->fg).to_rgba();
            auto bg = gl::Color(style->bg).to_rgba();
            ptr->console->fill(fg, bg);
            ptr->xpos = ptr->ypos = 0;
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    mrb_define_method(
        ruby, RConsole::rclass, "fill",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* ptr = mrb::self_to<RConsole>(self);
            auto n = mrb_get_argc(mrb);
            RStyle* style = &ptr->current_style;
            if (n == 1) { mrb_get_args(mrb, "d", &style, &RStyle::dt); }
            auto fg = gl::Color(style->fg).to_rgba();
            auto bg = gl::Color(style->bg).to_rgba();
            ptr->console->fill(fg, bg);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    mrb_define_method(
        ruby, RConsole::rclass, "scroll",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* ptr = mrb::self_to<RConsole>(self);
            int dx = 0;
            std::vector<mrb_value> rest;
            auto [dy] = mrb::get_args<int>(mrb, rest);
            if (!rest.empty()) { dx = mrb_fixnum(rest[0]); }
            ptr->scroll(dy, dx);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1) | MRB_ARGS_REST());

    mrb_define_method(
        ruby, RConsole::rclass, "print",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto n = mrb_get_argc(mrb);
            auto* ptr = mrb::self_to<RConsole>(self);
            const char* text = nullptr;
            RStyle* style = &ptr->current_style;
            RStyle temp_style;
            if (n == 1) {
                mrb_get_args(mrb, "z", &text);
            } else if (n == 2) {
                mrb_get_args(mrb, "zd", &text, &style, &RStyle::dt);
            } else if (n == 3) {
                mrb_value fg;
                mrb_value bg;
                mrb_get_args(mrb, "zoo", &text, &fg, &bg);
                style = &temp_style;
                style->fg = mrb::to_array<float, 4>(fg, mrb);
                style->bg = mrb::to_array<float, 4>(bg, mrb);
            }
            ptr->text(text, style);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1) | MRB_ARGS_REST());

    mrb_define_method(
        ruby, RConsole::rclass, "text",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto n = mrb_get_argc(mrb);
            auto* ptr = mrb::self_to<RConsole>(self);
            int x = 0;
            int y = 0;
            const char* text = nullptr;
            RStyle* style = &ptr->current_style;
            RStyle temp_style;
            if (n == 3) {
                mrb_get_args(mrb, "iiz", &x, &y, &text);
            } else if (n == 4) {
                mrb_get_args(mrb, "iizd", &x, &y, &text, &style, &RStyle::dt);
            } else if (n == 5) {
                mrb_value fg;
                mrb_value bg;
                mrb_get_args(mrb, "iizoo", &x, &y, &text, &fg, &bg);
                style = &temp_style;
                style->fg = mrb::to_array<float, 4>(fg, mrb);
                style->bg = mrb::to_array<float, 4>(bg, mrb);
            }
            ptr->text(x, y, text, style);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(3) | MRB_ARGS_REST());

    mrb_define_method(
        ruby, RConsole::rclass, "get_tile",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto chr = mrb::method(&RConsole::get, mrb, self);
            return mrb::to_value(static_cast<int>(chr), mrb);
        },
        MRB_ARGS_REQ(2));

    mrb_define_method(
        ruby, RConsole::rclass, "get_char",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto chr = mrb::method(&RConsole::get, mrb, self);
            auto str = utils::utf8_encode({chr, 0});
            return mrb::to_value(str, mrb);
        },
        MRB_ARGS_REQ(2));

    mrb_define_method(
        ruby, RConsole::rclass, "set_tile_size",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* ptr = mrb::self_to<RConsole>(self);
            auto [x, y] = mrb::get_args<int, int>(mrb);
            ptr->console->set_tile_size(x, y);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    mrb_define_method(
        ruby, RLayer::rclass, "get_tile_size",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* ptr = mrb::self_to<RConsole>(self);
            auto [tw, th] = ptr->console->get_char_size();
            std::array<int, 2> data{tw, th};
            return mrb::to_value(data, mrb);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, RConsole::rclass, "goto_xy",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* ptr = mrb::self_to<RConsole>(self);
            auto [x, y] = mrb::get_args<int, int>(mrb);
            ptr->xpos = x;
            ptr->ypos = y;
            fmt::print("Goto {} {}\n", x, y);
            // ptr->console->set_cursor(x, y);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    mrb_define_method(
        ruby, RConsole::rclass, "put_char",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* ptr = mrb::self_to<RConsole>(self);
            auto [x, y, c] = mrb::get_args<int, int, int>(mrb);
            ptr->console->put_char(x, y, c);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(3));

    mrb_define_method(
        ruby, RConsole::rclass, "clear_line",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* ptr = mrb::self_to<RConsole>(self);
            auto n = mrb_get_argc(mrb);
            int y = 0;
            RStyle* style = &ptr->current_style;
            if (n == 1) {
                mrb_get_args(mrb, "i", &y);
            } else {
                mrb_get_args(mrb, "id", &y, &style, &RStyle::dt);
            }
            auto fg = gl::Color(style->fg).to_rgba();
            auto bg = gl::Color(style->bg).to_rgba();
            ptr->console->clear_area(0, y, -1, 1, fg, bg);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    mrb_define_method(
        ruby, RConsole::rclass, "get_xy",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* ptr = mrb::self_to<RConsole>(self);
            std::array values{
                mrb::to_value(ptr->xpos, mrb), mrb::to_value(ptr->ypos, mrb)};
            return mrb_ary_new_from_values(mrb, 2, values.data());
        },
        MRB_ARGS_REQ(3));

    mrb_define_method(
        ruby, RConsole::rclass, "set_tile_image",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            uint32_t index = 0;
            RImage* image = nullptr;
            mrb_get_args(mrb, "id", &index, &image, &RImage::dt);
            auto* rconsole = mrb::self_to<RConsole>(self);
            rconsole->console->set_tile_image(index, image->texture);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(2));
}

void RConsole::update_tx()
{
    RLayer::update_tx();
    console->set_scale({scale[0], scale[1]});
    console->set_offset({trans[0], trans[1]});
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
    update_tx();

    current_style.fg = default_fg;
    current_style.bg = default_bg;

    clear();
}
