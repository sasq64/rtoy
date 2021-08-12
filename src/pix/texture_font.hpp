#pragma once

#include "font.hpp"
#include "gl/vec.hpp"
#include "tile_renderer.hpp"

#include <gl/texture.hpp>

#include <array>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct TextureFont
{
    struct TextAttrs
    {
        uint32_t fg = 0xffffffff;
        uint32_t bg = 0x000000ff;
        float scale = 1.0F;
    };

private:
    FTFont font;
    TileRenderer renderer;

    std::vector<uint32_t> data;
    std::vector<gl_wrap::TexRef> textures;

    using UV = std::array<vec2, 4>;

    static constexpr int texture_width = 256;
    static constexpr int texture_height = 512;

    bool needs_update = false;

    std::pair<int, int> next_pos;
    void add_char(char32_t c);

public:
    // Size of chars in font
    int char_width = 0;
    int char_height = 0;

    int tile_width = 0;

    explicit TextureFont(const char* name, int size = -1);

    void set_tile_size(int tw, int th);

    void add_tile(char32_t index, gl_wrap::TexRef texture);

    void clear();

    void render_text(std::pair<float, float> xy, TextAttrs const& attrs,
        std::string_view text);

    void render_text(std::pair<float, float> xy, TextAttrs const& attrs,
        std::u32string_view text32);

    void render();
};

