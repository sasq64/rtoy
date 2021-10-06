#include "rinput.hpp"

#include "error.hpp"
#include "keycodes.h"
#include "mrb_tools.hpp"
#include "settings.hpp"

#include <coreutils/utf8.h>
#include <mruby/class.h>

#include <fmt/format.h>

RInput::RInput(mrb_state* _ruby, System& _system) : ruby{_ruby}, system{_system}
{
    system.init_input(Settings{});
}

bool RInput::handle_event(KeyEvent const& e)
{
    auto code = e.key;
    fmt::print("CODE {:x}\n", e.key);
    if (code == RKEY_F12) {
        fmt::print("RESET!\n");
        do_reset = true;
        return true;
    }
    auto mod = e.mods;
    call_proc(ruby, key_handler, code, mod, e.device);
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
    if(last_frame != frame_counter) {
        if (me.buttons != 0) {
            //fmt::print("{:x} {} {}\n", me.buttons, me.x, me.y);
            call_proc(ruby, drag_handler, me.x, me.y);
        }
        mouse_x = me.x;
        mouse_y = me.y;
        last_frame = frame_counter;
    }
    return false;
}
bool RInput::handle_event(TextEvent const& me)
{
    fmt::print("TEXT '{}'\n", me.text);
    auto text32 = utils::utf8_decode(me.text);
    for (auto s : text32) {
        call_proc(ruby, key_handler, s, 0, me.device);
    }
    return false;
}

void RInput::put_char(char32_t c)
{
    call_proc(ruby, key_handler, c, 0, 0);
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
    frame_counter++;
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
    mrb_define_const(ruby, keys, "LEFT", mrb_int_value(ruby, RKEY_LEFT));
    mrb_define_const(ruby, keys, "RIGHT", mrb_int_value(ruby, RKEY_RIGHT));
    mrb_define_const(ruby, keys, "PAGE_UP", mrb_int_value(ruby, RKEY_PAGEUP));
    mrb_define_const(
        ruby, keys, "PAGE_DOWN", mrb_int_value(ruby, RKEY_PAGEDOWN));
    mrb_define_const(ruby, keys, "UP", mrb_int_value(ruby, RKEY_UP));
    mrb_define_const(ruby, keys, "DOWN", mrb_int_value(ruby, RKEY_DOWN));
    mrb_define_const(
        ruby, keys, "BACKSPACE", mrb_int_value(ruby, RKEY_BACKSPACE));
    mrb_define_const(ruby, keys, "ENTER", mrb_int_value(ruby, RKEY_ENTER));
    mrb_define_const(ruby, keys, "HOME", mrb_int_value(ruby, RKEY_HOME));
    mrb_define_const(ruby, keys, "END", mrb_int_value(ruby, RKEY_END));
    mrb_define_const(ruby, keys, "ESCAPE", mrb_int_value(ruby, RKEY_ESCAPE));
    mrb_define_const(ruby, keys, "TAB", mrb_int_value(ruby, RKEY_TAB));
    mrb_define_const(ruby, keys, "DEL", mrb_int_value(ruby, RKEY_DELETE));
    mrb_define_const(ruby, keys, "INSERT", mrb_int_value(ruby, RKEY_INSERT));
    mrb_define_const(ruby, keys, "END_", mrb_int_value(ruby, RKEY_END));
    mrb_define_const(
        ruby, keys, "LEFT_SHIFT", mrb_int_value(ruby, RKEY_LSHIFT));
    mrb_define_const(ruby, keys, "LEFT_ALT", mrb_int_value(ruby, RKEY_LALT));
    mrb_define_const(ruby, keys, "F1", mrb_int_value(ruby, RKEY_F1));
    mrb_define_const(ruby, keys, "F2", mrb_int_value(ruby, RKEY_F2));
    mrb_define_const(ruby, keys, "F3", mrb_int_value(ruby, RKEY_F3));
    mrb_define_const(ruby, keys, "F4", mrb_int_value(ruby, RKEY_F4));
    mrb_define_const(ruby, keys, "F5", mrb_int_value(ruby, RKEY_F5));
    mrb_define_const(ruby, keys, "F6", mrb_int_value(ruby, RKEY_F6));
    mrb_define_const(ruby, keys, "F7", mrb_int_value(ruby, RKEY_F7));
    mrb_define_const(ruby, keys, "F8", mrb_int_value(ruby, RKEY_F8));
    mrb_define_const(ruby, keys, "F9", mrb_int_value(ruby, RKEY_F9));
    mrb_define_const(ruby, keys, "F10", mrb_int_value(ruby, RKEY_F10));
    mrb_define_const(ruby, keys, "F11", mrb_int_value(ruby, RKEY_F11));
    mrb_define_const(ruby, keys, "F12", mrb_int_value(ruby, RKEY_F12));

    mrb_define_const(ruby, keys, "FIRE", mrb_int_value(ruby, RKEY_FIRE));

    rclass = mrb_define_class(ruby, "Input", ruby->object_class);
    MRB_SET_INSTANCE_TT(rclass, MRB_TT_DATA);

    default_input = new RInput(ruby, system);

    mrb_define_class_method(
        ruby, rclass, "default",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            return mrb::new_data_obj(mrb, default_input);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, rclass, "map",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* input = mrb::self_to<RInput>(self);
            auto [code, target, mods] = mrb::get_args<int, int, int>(mrb);
            //fmt::print("{:x} => {:x}\n", code, target);
            input->system.map_key(code, target, mods);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(3));

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
        ruby, rclass, "is_pressed",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* input = mrb::self_to<RInput>(self);
            auto [code] = mrb::get_args<int>(mrb);
            return mrb::to_value(input->system.is_pressed(code), mrb);
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
