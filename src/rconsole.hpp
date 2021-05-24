#pragma once
#include "rlayer.hpp"

#include "console.hpp"

#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>

#include <array>
#include <memory>

class GLConsole;
struct Cursor;
struct Style;

class RConsole : public RLayer
{
    int xpos = 0;
    int ypos = 0;

    void text(int x, int y, std::string const& t);
    void text(int x, int y, std::string const& t, uint32_t fg, uint32_t bg);
    void fill(uint32_t fg, uint32_t bg);
    void scroll(int dy, int dx);
    Console::Char get(int x, int y) const;
public:
    void text(std::string const& t);
    void text(std::string const& t, uint32_t fg, uint32_t bg);
    std::shared_ptr<GLConsole> console;

    RConsole(int width, int height, Style style);

    void reset() override;
    void clear();
    void render() override;

    static inline RClass* rclass = nullptr;
    static inline mrb_data_type dt{"Console",
        [](mrb_state*,
            void* data) { /*delete static_cast<GLConsole *>(data); */ }};

    static void reg_class(mrb_state* ruby);
};

