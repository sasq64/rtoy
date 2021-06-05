#include "rconsole.hpp"
#include "rimage.hpp"
#include "mrb_tools.hpp"
#include "pix/texture_font.hpp"
#include <mruby/array.h>

#include <coreutils/utf8.h>

#include <gl/program_cache.hpp>
#include <pix/gl_console.hpp>
#include <pix/pix.hpp>

RConsole::RConsole(int w, int h, Style style)
    : RLayer{w, h}, console(std::make_shared<GLConsole>(w, h, style))
{
    trans = {0.0F, 0.0F};
    scale = {2.0F, 2.0F};

    rot = 0.0F;
    update_tx();
}

void RConsole::clear()
{
    console->fill(console->default_style.fg, console->default_style.bg);
    console->flush();
    xpos = ypos = 0;
}

void RConsole::update_pos(Cursor const& cursor)
{
    xpos = cursor.x;
    ypos = cursor.y;
    int w = (float)width / console->font->char_width / scale[0];
    int h = (float)height / console->font->char_height / scale[1];
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
    console->default_style = {style.fg, style.bg};
    auto cursor = console->text(xpos, ypos, t);
    update_pos(cursor);
}

void RConsole::text(std::string const& t, uint32_t fg, uint32_t bg)
{
    auto cursor = console->text(xpos, ypos, t, fg, bg);
    update_pos(cursor);
}

void RConsole::text(int x, int y, std::string const& t)
{
    console->default_style = {style.fg, style.bg};
    console->text(x, y, t);
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
    auto& program = gl_wrap::ProgramCache::get_instance().textured;
    program.use();
    console->frame_buffer.bind();
    int ww = console->frame_buffer.width;
    int hh = console->frame_buffer.height;
    pix::set_colors(0xffffffff, 0);
    pix::set_transform(transform);
    pix::draw_quad_invy();
}

Console::Char RConsole::get(int x, int y) const
{
    return console->get(x, y);
}

void RConsole::reg_class(mrb_state* ruby)
{
    rclass = mrb_define_class(ruby, "Console", RLayer::rclass);

    MRB_SET_INSTANCE_TT(RConsole::rclass, MRB_TT_DATA);

    mrb_define_method(
        ruby, RConsole::rclass, "buffer",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* ptr = mrb::self_to<RConsole>(self);
            auto [bufno] = mrb::get_args<int>(mrb);
            if (bufno == ptr->current_buf) return mrb_nil_value();

            if (ptr->buffers.size() <= bufno) {
                ptr->buffers.resize(bufno + 1);
            }

            ptr->buffers[ptr->current_buf] = ptr->console->grid;

            ptr->current_buf = bufno;
            auto& buf = ptr->buffers[bufno];
            if (buf.empty()) { buf.resize(ptr->console->grid.size()); }
            ptr->console->grid = buf;

            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));
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
            ptr->clear();
            return mrb_nil_value();
        },
        MRB_ARGS_NONE());
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
                    mrb::get_args<std::string, uint32_t, uint32_t>(mrb);
                ptr->text(text, fg, bg);
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
                    mrb::get_args<int, int, std::string, uint32_t, uint32_t>(
                        mrb);
                ptr->text(x, y, text, fg, bg);
            }
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(3) | MRB_ARGS_REST());

    mrb_define_method(
        ruby, RConsole::rclass, "get_char",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto chr = mrb::method(&RConsole::get, mrb, self);
            std::u32string s = {chr.c, 0};
            auto str = utils::utf8_encode(s);
            return mrb::to_value(str, mrb);
        },
        MRB_ARGS_REQ(3));

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
        ruby, RConsole::rclass, "clear_line",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* ptr = mrb::self_to<RConsole>(self);
            auto [y] = mrb::get_args<int>(mrb);
            auto w = ptr->console->width;
            auto i = w * y;
            for (size_t x = 0; x < w; x++) {
                ptr->console->grid[i++] = {' ', ptr->console->default_style.fg,
                    ptr->console->default_style.bg};
            }
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    mrb_define_method(
        ruby, RConsole::rclass, "get_xy",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* ptr = mrb::self_to<RConsole>(self);
            mrb_value values[2] = {
                mrb::to_value(ptr->xpos, mrb), mrb::to_value(ptr->ypos, mrb)};
            return mrb_ary_new_from_values(mrb, 2, values);
        },
        MRB_ARGS_REQ(3));

    mrb_define_method(
        ruby, RConsole::rclass, "add_tile",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
          uint32_t index;
          RImage* image;
          mrb_get_args(mrb, "id", &index, &image, &RImage::dt);
          auto* rconsole = mrb::self_to<RConsole>(self);
          image->upload();
          rconsole->console->font->add_tile(index, image->texture);
          return mrb_nil_value();
        },
        MRB_ARGS_REQ(2));
}

void RConsole::update()
{
    console->default_style.bg = style.bg;
    console->default_style.fg = style.fg;
}

void RConsole::reset()
{
    transform = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    trans = {0.0F, 0.0F};
    scale = {2.0F, 2.0F};
    update_tx();
    console->font->clear();
}
