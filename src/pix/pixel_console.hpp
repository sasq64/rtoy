#pragma once

#include <gl/color.hpp>
#include <gl/functions.hpp>
#include <gl/gl.hpp>
#include <gl/program.hpp>
#include <gl/texture.hpp>

#include <pix/font.hpp>
#include <pix/pix.hpp>

#include <string>
#include <tuple>
#include <unordered_map>

class PixConsole
{
    static std::string vertex_shader;
    static std::string fragment_shader;
    int texture_width = 256 * 4;
    int texture_height = 256 * 4;
    int char_width = 13;
    int char_height = 24;

    static const int gap = 4;

    FTFont font;

    std::pair<int, int> next_pos{0, 0};

    std::unordered_map<char32_t, uint32_t> char_uvs;
    gl_wrap::Texture font_texture;
    gl_wrap::Texture uv_texture;
    gl_wrap::Texture col_texture;
    gl_wrap::Program program;

    std::vector<uint32_t> uvdata;
    std::vector<uint32_t> coldata;

    int width;
    int height;

    std::pair<float, float> scale{2.0, 2.0};
    std::pair<float, float> offset{0, 0};

    static constexpr std::pair<uint32_t, uint32_t> make_col(
        uint32_t fg, uint32_t bg)
    {
        bg >>= 8;
        return std::pair<uint32_t, uint32_t>{
            fg & 0xffff0000, ((bg & 0xff) << 16) | (bg & 0xff00) | (bg >> 16) |
                                 ((fg << 16) & 0xff000000)};
    }

    void add_char(char32_t c);

    std::pair<int, int> alloc_char(char32_t c);

public:
    PixConsole(int w, int h, std::string const& font_file = "data/bedstead.otf",
        int size = 32);

    void reset();

    std::pair<int, int> get_char_size();

    void set_tile_size(int w, int h);

    void set_tile_image(char32_t c, gl_wrap::TexRef tex);

    std::pair<int, int> text(int x, int y, std::string const& t);

    std::pair<int, int> text(
        int x, int y, std::string const& t, uint32_t fg, uint32_t bg);

    void flush();

    void put_char(int x, int y, char32_t c);

    uint32_t get_char(int x, int y);

    void put_color(int x, int y, uint32_t fg, uint32_t bg);

    void fill(uint32_t fg, uint32_t bg);

    void fill(uint32_t bg);

    void clear_area(
        int32_t x, int32_t y, int32_t w, int32_t h, uint32_t fg, uint32_t bg);

    void scroll(int dy, int dx);

    void set_scale(std::pair<float, float> s);

    void set_offset(std::pair<float, float> o);

    void render();
};
