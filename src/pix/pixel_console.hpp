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

struct ConsoleFont
{
    int texture_width = 256 * 4;
    int texture_height = 256 * 4;
    int char_width = 13;
    int char_height = 24;
    static const int gap = 4;
    FTFont font;
    std::pair<int, int> next_pos{0, 0};
    std::unordered_map<char32_t, uint32_t> char_uvs;
    gl_wrap::Texture font_texture;

    ConsoleFont(std::string const& font_file, int size);
    void set_tile_image(char32_t c, gl_wrap::TexRef tex);
    void set_tile_size(int w, int h);
    uint32_t get_offset(char32_t c);

    std::pair<float, float> get_uvscale() const;
    void add_char(char32_t c);
    std::pair<int, int> alloc_char(char32_t c);
};

class PixConsole
{
    static std::string vertex_shader;
    static std::string fragment_shader;

    gl_wrap::Program program;

    std::shared_ptr<ConsoleFont> font;

    int width;
    int height;

    gl_wrap::Texture uv_texture;
    gl_wrap::Texture col_texture;

    std::vector<uint32_t> uvdata;
    std::vector<uint32_t> coldata;

    static constexpr std::pair<uint32_t, uint32_t> make_col(
        uint32_t fg, uint32_t bg)
    {
        bg >>= 8;
        return std::pair<uint32_t, uint32_t>{
            fg & 0xffff0000, ((bg & 0xff) << 16) | (bg & 0xff00) | (bg >> 16) |
                                 ((fg << 16) & 0xff000000)};
    }

    void init();

public:
    PixConsole(int w, int h, std::string const& font_file = "data/bedstead.otf",
        int size = 32);

    PixConsole(int w, int h, std::shared_ptr<ConsoleFont> font);

    void reset();

    std::pair<int, int> get_char_size();

    void set_tile_size(int w, int h) { 
        font->set_tile_size(w, h);
        program.setUniform("uv_scale", font->get_uvscale());
    }
    void set_tile_image(char32_t c, gl_wrap::TexRef tex)
    {
        font->set_tile_image(c, tex);
    }

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

    void render(float ox, float oy, float sx, float sy);

    template <typename F, typename S>
    void render(F const& offset, S const& scale)
    {
        auto [ox, oy] = offset;
        auto [sx, sy] = scale;
        render(ox, oy, sx, sy);
    }
};
