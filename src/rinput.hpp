#pragma once

#include <cstdint>
#include <mruby.h>
#include <mruby/data.h>

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

public:
    explicit RInput(mrb_state* _ruby) : ruby{_ruby} {}

    void reset();
    bool update();
    bool should_reset();

    static inline RInput* default_input = nullptr;
    static inline RClass* rclass;
    static inline mrb_data_type dt{"Input", [](mrb_state*, void* data) {}};

    static void reg_class(mrb_state* ruby);
};
