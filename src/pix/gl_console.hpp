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

    //int32_t wrap_column = 1920/16;
    //int32_t scroll_line = 1080/32-1;

    //Cursor cursor{0, 0};

    std::shared_ptr<TextureFont> font;

    bool is_wide(char32_t) const override;

    gl_wrap::Texture frame_buffer;

    //void text(std::string const& t) override;
    //void text(std::string const& t, uint32_t fg, uint32_t bg);
    Cursor text(int x, int y, std::string const& t);
    Cursor text(int x, int y, std::string const& t, uint32_t fg, uint32_t bg);

//    void set_cursor(int x, int y)
//    {
//        cursor.x = x;
//        cursor.y = y;
//    }
//
//    Cursor get_cursor() const { return cursor; }

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

    GLConsole(int w, int h, Style _default_style);

    void flush() override;

    int get_width() override;
    int get_height() override;
    Char get(int x, int y) const override;
};

