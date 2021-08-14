#pragma once

#include "system.hpp"

#include <cstdint>
#include <mruby.h>
#include <mruby/data.h>

#include <unordered_map>

class RInput
{
    static uint32_t sdl2key(uint32_t code);

    bool do_reset = false;
    bool do_quit = false;
    mrb_state* ruby;
    mrb_value key_handler{};
    mrb_value click_handler{};
    mrb_value drag_handler{};

    void poll_events();

    int resize = 0;

    std::unordered_map<uint32_t, int> pressed;
    System& system;

    bool handle_event(QuitEvent const& e);
    bool handle_event(KeyEvent const& me);
    static bool handle_event(NoEvent const& me);
    bool handle_event(ClickEvent const& me);
    bool handle_event(MoveEvent const& me);
    bool handle_event(TextEvent const& me);

public:
    explicit RInput(mrb_state* _ruby, System& _system)
        : ruby{_ruby}, system{_system}
    {}

    void reset();
    bool update();
    bool should_reset();

    static inline RInput* default_input = nullptr;
    static inline RClass* rclass;
    static inline mrb_data_type dt{"Input", [](mrb_state*, void* data) {}};

    static void reg_class(mrb_state* ruby, System& system);
};
