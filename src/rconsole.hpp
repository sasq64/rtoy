#pragma once
#include "rlayer.hpp"

#include "console.hpp"

#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>

#include <array>
#include <memory>

class GLConsole;
class PixConsole;
struct Style;

class RConsole : public RLayer
{
    int xpos = 0;
    int ypos = 0;

    int current_buf = 0;

    void update_pos(std::pair<int, int> const& cursor);

    uint32_t get(int x, int y) const;
    std::array<float, 4> default_fg;
    std::array<float, 4> default_bg;
public:
    void text(int x, int y, std::string const& t);
    void text(int x, int y, std::string const& t, uint32_t fg, uint32_t bg);
    void fill(uint32_t fg, uint32_t bg);
    void scroll(int dy, int dx);
    void text(std::string const& t);
    void text(std::string const& t, uint32_t fg, uint32_t bg);
    //std::shared_ptr<GLConsole> console;
    std::shared_ptr<PixConsole> console;

    RConsole(int width, int height, Style style);

    void reset() override;
    void clear();
    void render() override;
    void update_tx() override;

    static inline RClass* rclass = nullptr;
    static inline mrb_data_type dt{"Console",
        [](mrb_state*,
            void* data) { /*delete static_cast<GLConsole *>(data); */ }};

    static void reg_class(mrb_state* ruby);
};

