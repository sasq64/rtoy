#pragma once

#include <cstdint>
#include <string>
#include <gl/vec.hpp>
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

class FontRenderer
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

    gl_wrap::Program program;
    // std::vector<uint32_t> data;
    //gl_wrap::Texture texture;
    gl_wrap::ArrayBuffer<GL_STREAM_DRAW> text_buffer;
    gl_wrap::ElementBuffer<GL_STATIC_DRAW> index_buffer;

    using UV = std::array<vec2, 4>;

    static constexpr int max_chars = 800;
    static constexpr int max_text_length = 256;

    size_t uv_buffer_pos;
    std::pair<int, int> next_pos;
    // std::unordered_set<char32_t> is_wide;
    std::vector<TextObj> objects;

public:
    std::unordered_map<char32_t, UV> uv_map;
    float char_width = 8;
    float char_height = 16;

    void add_char_location(char32_t c, int x, int y, int w, int h);

    bool has_char(char32_t c) const { return uv_map.count(c) > 0; }

    void add_text(std::pair<float, float> xy, TextAttrs const& attrs,
        std::string_view text);

    void add_text(std::pair<float, float> xy, TextAttrs const& attrs,
        std::u32string_view text32);

    void render();

    FontRenderer(int w, int h);
};
