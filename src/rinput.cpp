#include "rinput.hpp"

#include "error.hpp"
#include "keycodes.h"
#include "settings.hpp"

#include <coreutils/utf8.h>
#include <mrb/mrb_tools.hpp>

#include <fmt/format.h>

RInput::RInput(mrb_state* _ruby, System& _system) : ruby{_ruby}, system{_system}
{
    system.init_input(Settings{});
}

bool RInput::handle_event(KeyEvent const& e)
{
    auto code = e.key;
    // fmt::print("CODE {:x}\n", e.key);
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
    if (last_frame != frame_counter) {
        if (me.buttons != 0) {
            // fmt::print("{:x} {} {}\n", me.buttons, me.x, me.y);
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
    // fmt::print("TEXT '{}'\n", me.text);
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
    key_handler.clear();
    click_handler.clear();
    drag_handler.clear();
}

struct Key
{};

void RInput::reg_class(mrb_state* ruby, System& system)
{
    mrb::make_module<Key>(ruby, "Key");

    mrb::define_const<Key>(ruby, "LEFT", RKEY_LEFT);
    mrb::define_const<Key>(ruby, "RIGHT", RKEY_RIGHT);
    mrb::define_const<Key>(ruby, "PAGE_UP", RKEY_PAGEUP);
    mrb::define_const<Key>(ruby, "PAGE_DOWN", RKEY_PAGEDOWN);
    mrb::define_const<Key>(ruby, "UP", RKEY_UP);
    mrb::define_const<Key>(ruby, "DOWN", RKEY_DOWN);
    mrb::define_const<Key>(ruby, "BACKSPACE", RKEY_BACKSPACE);
    mrb::define_const<Key>(ruby, "ENTER", RKEY_ENTER);
    mrb::define_const<Key>(ruby, "HOME", RKEY_HOME);
    mrb::define_const<Key>(ruby, "END", RKEY_END);
    mrb::define_const<Key>(ruby, "ESCAPE", RKEY_ESCAPE);
    mrb::define_const<Key>(ruby, "TAB", RKEY_TAB);
    mrb::define_const<Key>(ruby, "DEL", RKEY_DELETE);
    mrb::define_const<Key>(ruby, "INSERT", RKEY_INSERT);
    mrb::define_const<Key>(ruby, "END_", RKEY_END);
    mrb::define_const<Key>(ruby, "LEFT_SHIFT", RKEY_LSHIFT);
    mrb::define_const<Key>(ruby, "LEFT_ALT", RKEY_LALT);
    mrb::define_const<Key>(ruby, "F1", RKEY_F1);
    mrb::define_const<Key>(ruby, "F2", RKEY_F2);
    mrb::define_const<Key>(ruby, "F3", RKEY_F3);
    mrb::define_const<Key>(ruby, "F4", RKEY_F4);
    mrb::define_const<Key>(ruby, "F5", RKEY_F5);
    mrb::define_const<Key>(ruby, "F6", RKEY_F6);
    mrb::define_const<Key>(ruby, "F7", RKEY_F7);
    mrb::define_const<Key>(ruby, "F8", RKEY_F8);
    mrb::define_const<Key>(ruby, "F9", RKEY_F9);
    mrb::define_const<Key>(ruby, "F10", RKEY_F10);
    mrb::define_const<Key>(ruby, "F11", RKEY_F11);
    mrb::define_const<Key>(ruby, "F12", RKEY_F12);
    mrb::define_const<Key>(ruby, "FIRE", RKEY_FIRE);

    mrb::make_noinit_class<RInput>(ruby, "Input");
    mrb::set_deleter<RInput>(ruby, [](mrb_state*, void*) {});

    default_input = new RInput(ruby, system);

    mrb::add_class_method<RInput>(
        ruby, "default", []() { return default_input; });

    mrb::add_method<RInput>(
        ruby, "map", [](RInput* self, int code, int target, int mods) {
            self->system.map_key(code, target, mods);
        });

    mrb::add_method<RInput>(
        ruby, "is_pressed", [](RInput* self, mrb::ArgN n, int code, int dev) {
            if (n == 1) { dev = -1; }
            return self->system.is_pressed(code, dev);
        });
    mrb::add_method<RInput>(ruby, "get_modifiers", [](RInput*) { return 0; });

    mrb::add_method<RInput>(
        ruby, "on_key", [](RInput* self, mrb::Block callback) {
            self->key_handler = callback;
        });
    mrb::add_method<RInput>(
        ruby, "on_drag", [](RInput* self, mrb::Block callback) {
            self->drag_handler = callback;
        });
    mrb::add_method<RInput>(
        ruby, "on_click", [](RInput* self, mrb::Block callback) {
            self->click_handler = callback;
        });
}
