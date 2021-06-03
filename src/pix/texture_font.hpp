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

    struct TextObj
    {
        std::pair<float, float> xy;
        size_t buffer_offset;
        size_t size;
        TextAttrs attrs;
    };

    static inline const std::string vertex_shader =
        R"gl(
    #ifdef GL_ES
        precision mediump float;
    #endif
        attribute vec2 in_pos;
        attribute vec2 in_uv;
        uniform float scale;
        uniform vec2 screen_scale;
        uniform vec2 xypos;
        uniform vec2 tex_scale;
        varying vec2 out_uv;
        void main() {
            out_uv = in_uv * tex_scale;
            gl_Position = vec4((in_pos * scale + xypos) * screen_scale, 0, 1);
        }
    )gl";

    static inline const std::string pixel_shader =
        R"gl(
    #ifdef GL_ES
        precision mediump float;
    #endif
        uniform sampler2D in_tex;
        varying vec2 out_uv;
        uniform vec4 fg_color;
        uniform vec4 bg_color;
        void main() {
            //gl_FragColor = vec4(out_uv, 0, 1);
            vec4 col = texture2D(in_tex, out_uv);
            float a = col.a;
              //  gl_FragColor = vec4(fg_color.rgb * a +
                //    bg_color.rgb * (1.0 - a), 1.0);
            gl_FragColor = fg_color * col + bg_color * (1.0 - a);
             
        }
    )gl";

    FTFont font;
    FontRenderer renderer;

    std::vector<uint32_t> data;
    gl_wrap::Texture texture;

    using UV = std::array<vec2, 4>;
    //std::unordered_map<char32_t, UV> uv_map;

    static constexpr int max_chars = 800;
    static constexpr int max_text_length = 256;
    static constexpr int texture_width = 256;
    static constexpr int texture_height = 512;
    int char_width = 0;
    int char_height = 0;

    bool needs_update = false;

    std::pair<int, int> next_pos;
    std::unordered_set<char32_t> is_wide;

    explicit TextureFont(const char* name, int size = -1);

    void add_char_image(char32_t c, uint32_t* pixels);
    void add_char(char32_t c);

    void add_text(std::pair<float, float> xy, TextAttrs const& attrs,
        std::string_view text);

    void add_text(std::pair<float, float> xy, TextAttrs const& attrs,
        std::u32string_view text32);

    void render();
};

