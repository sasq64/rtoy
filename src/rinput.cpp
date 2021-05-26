#include "rinput.hpp"

#include "SDL_video.h"
#include "error.hpp"
#include "keycodes.h"
#include "mrb_tools.hpp"

#include <mruby/class.h>

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_keycode.h>
#include <fmt/format.h>

uint32_t RInput::sdl2key(uint32_t code)
{
    switch (code) {
    case SDLK_LEFT: return KEY_LEFT;
    case SDLK_RIGHT: return KEY_RIGHT;
    case SDLK_PAGEUP: return KEY_PAGEUP;
    case SDLK_PAGEDOWN: return KEY_PAGEDOWN;
    case SDLK_UP: return KEY_UP;
    case SDLK_DOWN: return KEY_DOWN;
    case SDLK_END: return KEY_END;
    case SDLK_HOME: return KEY_HOME;
    case SDLK_ESCAPE: return KEY_ESCAPE;
    case SDLK_RETURN: return KEY_ENTER;
    case SDLK_INSERT: return KEY_INSERT;
    case SDLK_DELETE: return KEY_DELETE;
    case SDLK_F1: return KEY_F1;
    case SDLK_F5: return KEY_F5;
    case SDLK_F7: return KEY_F7;
    case SDLK_F3: return KEY_F3;
    default: return code;
    }
}
void RInput::poll_events()
{
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_MOUSEMOTION) {
            auto& me = e.motion;
            if (me.state != 0) {
                fmt::print("{:x} {} {}\n", me.state, me.x, me.y);
                call_proc(ruby, drag_handler, me.x, me.y);
            }
        } else if (e.type == SDL_MOUSEBUTTONDOWN) {
            call_proc(ruby, click_handler, e.button.x, e.button.y);
        } else if (e.type == SDL_TEXTINPUT) {
            fmt::print("TEXT '{}'\n", e.text.text);
            call_proc(ruby, key_handler, e.text.text[0], 0);
        } else if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_F12) {
                do_reset = true;
                continue;
            }
            auto& ke = e.key;
            auto code = sdl2key(ke.keysym.sym);
            auto mod = ke.keysym.mod;
            fmt::print("KEY {:x}\n", ke.keysym.sym);
            fmt::print("MOD {:x}\n", mod);
            if (code < 0x20 || code > 0x1fffc || (mod & 0xc0) != 0) {
                call_proc(ruby, key_handler, code, mod);
            }
        } else if (e.type == SDL_QUIT) {
            fmt::print("quit\n");
            do_quit = true;
        } else if (e.type == SDL_WINDOWEVENT) {
            if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                SDL_Log("Window %d resized to %dx%d", e.window.windowID,
                    e.window.data1, e.window.data2);
                resize = 50;
            }
        }
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
    poll_events();
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
void RInput::reg_class(mrb_state* ruby)
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

    default_input = new RInput(ruby);

    mrb_define_class_method(
        ruby, rclass, "default",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            return mrb::new_data_obj(mrb, default_input);
        },
        MRB_ARGS_NONE());

    mrb_define_class_method(
        ruby, rclass, "get_clipboard",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            const char* clip = SDL_GetClipboardText();
            if (clip == nullptr) { return mrb_nil_value(); }
            return mrb::to_value(clip, mrb);
        },
        MRB_ARGS_NONE());

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
