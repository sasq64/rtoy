#include "rconsole.hpp"
#include "mrb_tools.hpp"
#include "pix/texture_font.hpp"
#include "rimage.hpp"

#include "pix/pixel_console.hpp"

#include <mruby/array.h>

#include <coreutils/utf8.h>

#include <gl/program_cache.hpp>
#include <pix/gl_console.hpp>
#include <pix/pix.hpp>

RConsole::RConsole(int w, int h, Style style)
    : RLayer{w, h},
      console(
          std::make_shared<PixConsole>(256, 256, style.font, style.font_size))
{
    default_fg = this->style.fg = gl::Color(style.fg).to_array();
    default_bg = this->style.bg = gl::Color(style.bg).to_array();
    reset();
    /* trans = {0.0F, 0.0F}; */
    /* scale = {2.0F, 2.0F}; */

    /* rot = 0.0F; */
    /* update_tx(); */
}

void RConsole::clear()
{
    auto fg = gl::Color(style.fg).to_rgba();
    auto bg = gl::Color(style.bg).to_rgba();
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

void RConsole::text(std::string const& t)
{
    auto fg = gl::Color(style.fg).to_rgba();
    auto bg = gl::Color(style.bg).to_rgba();
    auto cursor = console->text(xpos, ypos, t, fg, bg);
    update_pos(cursor);
}

void RConsole::text(std::string const& t, uint32_t fg, uint32_t bg)
{
    auto cursor = console->text(xpos, ypos, t, fg, bg);
    update_pos(cursor);
}

void RConsole::text(int x, int y, std::string const& t)
{
    auto fg = gl::Color(style.fg).to_rgba();
    auto bg = gl::Color(style.bg).to_rgba();
    console->text(x, y, t, fg, bg);
}

void RConsole::text(
    int x, int y, std::string const& t, uint32_t fg, uint32_t bg)
{
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
    console->flush();
    console->render();
    /* auto& program = gl_wrap::ProgramCache::get_instance().textured; */
    /* program.use(); */
    /* console->frame_buffer.bind(); */
    /* pix::set_colors(0xffffffff, 0); */
    /* pix::set_transform(transform); */
    /* gl::ProgramCache::get_instance().textured.use(); */
    /* pix::draw_quad_invy(); */
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
        [](mrb_state* /*mrb*/, mrb_value self) -> mrb_value {
            auto* ptr = mrb::self_to<RConsole>(self);
            ptr->clear();
            return mrb_nil_value();
        },
        MRB_ARGS_NONE());
    mrb_define_method(
        ruby, RConsole::rclass, "fill_bg",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* ptr = mrb::self_to<RConsole>(self);
            auto [bg] = mrb::get_args<mrb_value>(mrb);
            ptr->style.bg = mrb::to_array<float, 4>(bg, mrb);
            auto bcol = gl::Color(ptr->style.bg);
            ptr->console->fill(bcol.to_rgba());
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
            std::vector<mrb_value> rest;
            auto n = mrb_get_argc(mrb);
            auto* ptr = mrb::self_to<RConsole>(self);
            if (n == 1) {
                auto [text] = mrb::get_args<std::string>(mrb);
                ptr->text(text);
            } else if (n == 3) {
                auto [text, fg, bg] =
                    mrb::get_args<std::string, mrb_value, mrb_value>(mrb);
                auto fcol = gl::Color(mrb::to_array<float, 4>(fg, mrb));
                auto bcol = gl::Color(mrb::to_array<float, 4>(bg, mrb));
                ptr->text(text, fcol.to_rgba(), bcol.to_rgba());
            }
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1) | MRB_ARGS_REST());

    mrb_define_method(
        ruby, RConsole::rclass, "text",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            std::vector<mrb_value> rest;
            auto n = mrb_get_argc(mrb);
            auto* ptr = mrb::self_to<RConsole>(self);
            if (n == 3) {
                auto [x, y, text] = mrb::get_args<int, int, std::string>(mrb);
                ptr->text(x, y, text);
            } else if (n == 5) {
                auto [x, y, text, fg, bg] =
                    mrb::get_args<int, int, std::string, mrb_value, mrb_value>(
                        mrb);
                mrb::get_args<int, int, std::string, mrb_value, mrb_value>(mrb);
                auto fcol = gl::Color(mrb::to_array<float, 4>(fg, mrb));
                auto bcol = gl::Color(mrb::to_array<float, 4>(bg, mrb));
                ptr->text(x, y, text, fcol.to_rgba(), bcol.to_rgba());
            }
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
            std::u32string s = {chr, 0};
            auto str = utils::utf8_encode(s);
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
            std::array<unsigned, 2> data{tw, th};
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
            auto [y] = mrb::get_args<int>(mrb);
            auto fg = gl::Color(ptr->style.fg).to_rgba();
            auto bg = gl::Color(ptr->style.bg).to_rgba();
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
        ruby, RConsole::rclass, "add_tile",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            uint32_t index = 0;
            RImage* image = nullptr;
            mrb_get_args(mrb, "id", &index, &image, &RImage::dt);
            auto* rconsole = mrb::self_to<RConsole>(self);
            auto& img = image->image;
            auto x = std::lround(image->x());
            auto y = std::lround(image->y());
            auto w = std::lround(image->width());
            auto h = std::lround(image->height());
            fmt::print("{} {} {} {}\n", image->x(), image->y(), image->width(),
                image->height());
            image->upload();
            rconsole->console->set_tile_image(index, image->texture);
            //rconsole->console->set_tile_image(index, img, x, y, w, h);
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
    float s = 1.0;
    int total = lines;
    while (total > 50) {
        s += 1.0;
        total -= lines;
    }

    scale = {s, s};
    update_tx();

    style.fg = default_fg;
    style.bg = default_bg;

    clear();
}
