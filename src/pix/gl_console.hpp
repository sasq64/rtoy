#pragma once

#include "console.hpp"
#include <gl/texture.hpp>

#include <memory>
#include <string_view>
#include <vector>

struct TextureFont;

struct Style
{
    uint32_t fg;
    uint32_t bg;
    std::string font;
    int font_size;
};

/* struct Cursor */
/* { */
/*     int x; */
/*     int y; */
/* }; */
using Cursor = std::pair<int, int>;

struct GLConsole
{
    using Char = Console::Char;

    std::vector<Char> grid;
    std::vector<Char> old_grid;

    int32_t width = 0;
    int32_t height = 0;

    int tile_width = 0;
    int tile_height = 0;

    std::vector<int> dirty;

    std::shared_ptr<TextureFont> font;

    gl_wrap::Texture frame_buffer;

    void set_tile_size(int tw, int th);

    //Cursor text(int x, int y, std::string const& t);
    Cursor text(int x, int y, std::string const& t, uint32_t fg, uint32_t bg);

    void put_char(int x, int y, char32_t c);
    void put_char(Cursor cursor, char32_t c)
    {
        put_char(cursor.first, cursor.second, c);
    }

    void put_color(int x, int y, uint32_t fg, uint32_t bg);

    void fill(uint32_t fg, uint32_t bg);

    void scroll(int dy, int dx);

    void blit(
        int x, int y, int stride, std::vector<Char> const& source);

    // Creates a render target of w*h pixels for tile rendering
    GLConsole(int w, int h, Style _default_style);

    void flush();

    void reset();

    int get_width();
    int get_height();
    Char get(int x, int y) const;
};

