#pragma once

#include "font.hpp"
#include "font_renderer.hpp"
#include "gl/vec.hpp"

#include <gl/buffer.hpp>
#include <gl/color.hpp>
#include <gl/functions.hpp>
#include <gl/program_cache.hpp>
#include <gl/shader.hpp>
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
    FontRenderer renderer;

    std::vector<uint32_t> data;
    gl_wrap::TexRef texture;

    using UV = std::array<vec2, 4>;

    static constexpr int texture_width = 256;
    static constexpr int texture_height = 512;

    bool needs_update = false;

    std::pair<int, int> next_pos;
    void add_char(char32_t c);
public:
    int char_width = 0;
    int char_height = 0;
    std::unordered_set<char32_t> is_wide;
    explicit TextureFont(const char* name, int size = -1);

    void add_text(std::pair<float, float> xy, TextAttrs const& attrs,
        std::string_view text);

    void add_text(std::pair<float, float> xy, TextAttrs const& attrs,
        std::u32string_view text32);

    void render();
};

