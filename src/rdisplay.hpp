#pragma once

#include "rlayer.hpp"

#include <mruby.h>
#include <mruby/data.h>
#include "mrb_tools.hpp"

#include <array>
#include <memory>
#include <string>

struct SDL_Window;
struct RConsole;
struct RCanvas;
struct RSprites;

class Display : public RLayer
{

    mrb_value draw_handler{};

    mrb_value disp_obj{};

    mrb_state* ruby = nullptr;
    SDL_Window* window = nullptr;

    int w = 1440;
    int h = 960;
    std::array<float, 4> bg = {0,0, 0.8, 1.0};
    std::array<float, 16> Id = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    std::shared_ptr<RCanvas> canvas;
    std::shared_ptr<RSprites> sprites;
public:
    static inline bool full_screen = false;
    std::shared_ptr<RConsole> console;
    static inline RClass* rclass;
    static mrb_data_type dt;
    static inline Display* default_display = nullptr;
    explicit Display(mrb_state* state);

    void setup();
    void reset() override;
    bool begin_draw();
    void end_draw();

    static void reg_class(mrb_state* ruby);

};

