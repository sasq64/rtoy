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
};

struct Cursor
{
    int x;
    int y;
};

struct GLConsole : public Console
{
    std::vector<Char> grid;
    std::vector<Char> old_grid;

    Style default_style;

    int32_t width = 0;
    int32_t height = 0;

    int tile_width = 0;
    int tile_height = 0;

    std::vector<int> dirty;

    std::shared_ptr<TextureFont> font;

    bool is_wide(char32_t) const override;

    gl_wrap::Texture frame_buffer;

    void set_tile_size(int tw, int th);

    Cursor text(int x, int y, std::string const& t);
    Cursor text(int x, int y, std::string const& t, uint32_t fg, uint32_t bg);

    void put_char(int x, int y, char32_t c) override;
    void put_char(Cursor cursor, char32_t c)
    {
        put_char(cursor.x, cursor.y, c);
    }

    void put_color(int x, int y, uint32_t fg, uint32_t bg) override;

    void fill(uint32_t fg, uint32_t bg) override;

    void scroll(int dy, int dx);

    void blit(
        int x, int y, int stride, std::vector<Char> const& source) override;

    // Creates a render target of w*h pixels for tile rendering
    GLConsole(int w, int h, Style _default_style);

    void flush() override;

    int get_width() override;
    int get_height() override;
    Char get(int x, int y) const override;
};

