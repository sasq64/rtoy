#pragma once

#include "rlayer.hpp"
#include "settings.hpp"
#include "system.hpp"

#include "mrb_tools.hpp"
#include <mruby.h>
#include <mruby/data.h>

#include <array>
#include <memory>
#include <string>

class RConsole;
class RCanvas;
class RSprites;
class RSprite;

class Display : public RLayer
{
    mrb_value draw_handler{};
    mrb_value disp_obj{};
    mrb_state* ruby = nullptr;
    std::shared_ptr<Screen> window = nullptr;

    Settings const& settings;

    std::array<float, 4> bg = {0.0F, 0.0F, 0.8F, 1.0F};
    std::array<float, 16> Id = {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F};

    std::shared_ptr<RCanvas> canvas;
    std::shared_ptr<RSprites> sprites;

    std::vector<std::shared_ptr<RLayer>> layers;

public:
    RSprite* mouse_cursor = nullptr;
    std::shared_ptr<RConsole> console;
    static inline RClass* rclass;
    static mrb_data_type dt;
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

