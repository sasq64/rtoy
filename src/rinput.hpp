#pragma once

#include "system.hpp"

#include <mrb/mrb_tools.hpp>

#include <cstdint>
#include <unordered_map>

class RInput
{
    static uint32_t sdl2key(uint32_t code);

    bool do_reset = false;
    bool do_quit = false;
    mrb_state* ruby;
    mrb::Value key_handler{};
    mrb::Value click_handler{};
    mrb::Value drag_handler{};

    int resize = 0;
    int mouse_x = 0;
    int mouse_y = 0;
    int64_t frame_counter = 0;
    int64_t last_frame = 0;

    std::unordered_map<uint32_t, int> pressed;
    System& system;

    bool handle_event(QuitEvent const& e);
    bool handle_event(KeyEvent const& me);
    static bool handle_event(NoEvent const& me);
    bool handle_event(ClickEvent const& me);
    bool handle_event(MoveEvent const& me);
    bool handle_event(TextEvent const& me);

public:
    explicit RInput(mrb_state* _ruby, System& _system);

    void put_char(char32_t c);

    void reset();
    bool update();
    bool should_reset();

    std::pair<int, int> mouse_pos() { return { mouse_x, mouse_y }; }

    static inline RInput* default_input = nullptr;

    static void reg_class(mrb_state* ruby, System& system);
};
