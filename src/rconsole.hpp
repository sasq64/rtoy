#pragma once
#include "rlayer.hpp"

#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>

#include <array>
#include <memory>

class GLConsole;
class PixConsole;

struct Style
{
    uint32_t fg;
    uint32_t bg;
    std::string font;
    int font_size;
};

class RConsole : public RLayer
{
    int xpos = 0;
    int ypos = 0;

    void update_pos(std::pair<int, int> const& cursor);

    uint32_t get(int x, int y) const;
    std::array<float, 4> default_fg{};
    std::array<float, 4> default_bg{};

public:
    std::shared_ptr<PixConsole> console;

    void text(
        int x, int y, std::string const& t, RStyle const* style = nullptr);
    void fill(uint32_t fg, uint32_t bg);
    void scroll(int dy, int dx);
    void text(std::string const& t, RStyle const* style = nullptr);

    RConsole(int width, int height, Style const& style);
    RConsole(int width, int height, Style const& style,
        std::shared_ptr<PixConsole> console);

    void reset() override;
    void clear() override;
    void render(RLayer const* parent) override;

    static inline RClass* rclass = nullptr;
    //static inline mrb_data_type dt{"Console", [](mrb_state*, void* data) {}};

    static void reg_class(mrb_state* ruby);
};

