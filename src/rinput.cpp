#include "rinput.hpp"

#include "error.hpp"
#include "keycodes.h"
#include "mrb_tools.hpp"

#include <coreutils/utf8.h>
#include <mruby/class.h>

#include <fmt/format.h>

bool RInput::handle_event(KeyEvent const& e)
{
    auto code = e.key;
    if (code == KEY_F12) {
        do_reset = true;
        return true;
    }
    auto mod = e.mods;
    if (code < 0x20 || code > 0x1fffc || (mod & 0xc0) != 0) {
        call_proc(ruby, key_handler, code, mod);
    }
    return false;
}

bool RInput::handle_event(QuitEvent const&)
{
    do_quit = true;
    return true;
}

bool RInput::handle_event(NoEvent const&)
{
    return true;
}
bool RInput::handle_event(ClickEvent const& me)
{
    call_proc(ruby, click_handler, me.x, me.y);
    return false;
}
bool RInput::handle_event(MoveEvent const& me)
{
    if (me.buttons != 0) {
        fmt::print("{:x} {} {}\n", me.buttons, me.x, me.y);
        call_proc(ruby, drag_handler, me.x, me.y);
    }
    return false;
}
bool RInput::handle_event(TextEvent const& me)
{
    fmt::print("TEXT '{}'\n", me.text);
    auto text32 = utils::utf8_decode(me.text);
    for (auto s : text32) {
        call_proc(ruby, key_handler, s, 0);
    }
    return false;
}

void RInput::poll_events()
{
    bool done = false;
    while (!done) {
        auto event = system.poll_events();
        done =
            std::visit([&](auto const& e) { return handle_event(e); }, event);
    }
}
bool RInput::should_reset()
{
    auto res = do_reset;
    do_reset = false;
    return res;
}
bool RInput::update()
{
    while (!std::visit([&](auto const& e) { return handle_event(e); },
        system.poll_events())) {}
    if (resize > 0) {
        resize--;
        if (resize == 0) { do_reset = true; }
    }
    return do_quit;
}
void RInput::reset()
{
    SET_NIL_VALUE(key_handler);
    SET_NIL_VALUE(click_handler);
    SET_NIL_VALUE(drag_handler);
}
void RInput::reg_class(mrb_state* ruby, System& system)
{
    auto* keys = mrb_define_module(ruby, "Key");
    mrb_define_const(ruby, keys, "LEFT", mrb_fixnum_value(KEY_LEFT));
    mrb_define_const(ruby, keys, "RIGHT", mrb_fixnum_value(KEY_RIGHT));
    mrb_define_const(ruby, keys, "PAGE_UP", mrb_fixnum_value(KEY_PAGEUP));
    mrb_define_const(ruby, keys, "PAGE_DOWN", mrb_fixnum_value(KEY_PAGEDOWN));
    mrb_define_const(ruby, keys, "UP", mrb_fixnum_value(KEY_UP));
    mrb_define_const(ruby, keys, "DOWN", mrb_fixnum_value(KEY_DOWN));
    mrb_define_const(ruby, keys, "BACKSPACE", mrb_fixnum_value(KEY_BACKSPACE));
    mrb_define_const(ruby, keys, "ENTER", mrb_fixnum_value(KEY_ENTER));
    mrb_define_const(ruby, keys, "HOME", mrb_fixnum_value(KEY_HOME));
    mrb_define_const(ruby, keys, "END", mrb_fixnum_value(KEY_END));
    mrb_define_const(ruby, keys, "ESCAPE", mrb_fixnum_value(KEY_ESCAPE));
    mrb_define_const(ruby, keys, "TAB", mrb_fixnum_value(KEY_TAB));
    mrb_define_const(ruby, keys, "DEL", mrb_fixnum_value(KEY_DELETE));
    mrb_define_const(ruby, keys, "INSERT", mrb_fixnum_value(KEY_INSERT));
    mrb_define_const(ruby, keys, "END_", mrb_fixnum_value(KEY_END));
    mrb_define_const(ruby, keys, "F1", mrb_fixnum_value(KEY_F1));
    mrb_define_const(ruby, keys, "F3", mrb_fixnum_value(KEY_F3));
    mrb_define_const(ruby, keys, "F5", mrb_fixnum_value(KEY_F5));
    mrb_define_const(ruby, keys, "F7", mrb_fixnum_value(KEY_F7));

    rclass = mrb_define_class(ruby, "Input", ruby->object_class);
    MRB_SET_INSTANCE_TT(rclass, MRB_TT_DATA);

    default_input = new RInput(ruby, system);

    mrb_define_class_method(
        ruby, rclass, "default",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            return mrb::new_data_obj(mrb, default_input);
        },
        MRB_ARGS_NONE());

    mrb_define_class_method(
        ruby, rclass, "get_clipboard",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            // TODO
            const char* clip = nullptr; // SDL_GetClipboardText();
            if (clip == nullptr) { return mrb_nil_value(); }
            return mrb::to_value(clip, mrb);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, rclass, "get_key",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* input = mrb::self_to<RInput>(self);
            auto [code] = mrb::get_args<int>(mrb);
            return mrb::to_value(input->pressed[code] == 1, mrb);
        },
        MRB_ARGS_BLOCK());

    mrb_define_method(
        ruby, rclass, "get_modifiers",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* input = mrb::self_to<RInput>(self);
            // TODO
            uint32_t mods = 0; // SDL_GetModState();
            return mrb::to_value(mods, mrb);
        },
        MRB_ARGS_BLOCK());

    mrb_define_method(
        ruby, rclass, "on_key",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* input = mrb::self_to<RInput>(self);
            mrb_get_args(mrb, "&!", &input->key_handler);
            mrb_gc_register(mrb, input->key_handler);
            return mrb_nil_value();
        },
        MRB_ARGS_BLOCK());
    mrb_define_method(
        ruby, rclass, "on_drag",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* input = mrb::self_to<RInput>(self);
            mrb_get_args(mrb, "&!", &input->drag_handler);
            mrb_gc_register(mrb, input->drag_handler);
            return mrb_nil_value();
        },
        MRB_ARGS_BLOCK());
    mrb_define_method(
        ruby, rclass, "on_click",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* input = mrb::self_to<RInput>(self);
            mrb_get_args(mrb, "&!", &input->click_handler);
            mrb_gc_register(mrb, input->click_handler);
            return mrb_nil_value();
        },
        MRB_ARGS_BLOCK());
}
