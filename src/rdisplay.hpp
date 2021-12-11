#pragma once

#include "rlayer.hpp"
#include "settings.hpp"
#include "system.hpp"

#include <mrb/mrb_tools.hpp>

#include <chrono>

#include <array>
#include <memory>
#include <string>

class RConsole;
class RCanvas;
class RSprites;
class RSprite;
class PixConsole;

class Display : public RLayer
{
    mrb::Value draw_handler{};
    mrb::Value disp_obj{};
    mrb_state* ruby = nullptr;
    std::shared_ptr<Screen> window = nullptr;

    Settings const& settings;

    std::array<float, 4> bg = {0.0F, 0.0F, 0.8F, 1.0F};
    std::array<float, 16> Id = {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F};

    std::shared_ptr<RCanvas> canvas;
    std::shared_ptr<RSprites> sprite_field;

    std::vector<std::shared_ptr<RLayer>> layers;

    std::shared_ptr<PixConsole> debug_console;
    bool debug_on = false;

    std::array<int64_t, 10> bench_times;
    std::chrono::time_point<std::chrono::steady_clock> bench_start;

    int64_t swap_t = 0;

    std::vector<int32_t> dump(int x, int y, int w, int h);
    static int32_t dump(int x, int y);

public:
    int64_t pre_t = 0;
    RSprite* mouse_cursor = nullptr;
    std::shared_ptr<RConsole> console;
    mrb_value consoles{};
    mrb_value sprite_fields{};
    mrb_value canvases{};
    // static mrb_data_type dt;
    static inline Display* default_display = nullptr;
    explicit Display(
        mrb_state* state, System& system, Settings const& _settings);

    void setup();
    void reset() override;
    bool begin_draw();
    void end_draw();
    void swap();

    static void reg_class(
        mrb_state* ruby, System& system, Settings const& settings);
};

