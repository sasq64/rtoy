#pragma once

#include <cstdint>
#include <gl/buffer.hpp>
#include <gl/color.hpp>
#include <gl/functions.hpp>
#include <gl/program_cache.hpp>
#include <gl/shader.hpp>
#include <gl/texture.hpp>
#include <gl/vec.hpp>
#include <string>

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
        varying vec2 out_uv;
        void main() {
            out_uv = in_uv;
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
            vec4 col = texture2D(in_tex, out_uv);
            float a = col.a;
            gl_FragColor = fg_color * col + bg_color * (1.0 - a);
             
        }
    )gl";

    gl_wrap::Program program;
    gl_wrap::ArrayBuffer<GL_STREAM_DRAW> text_buffer;
    gl_wrap::ElementBuffer<GL_STATIC_DRAW> index_buffer;

    using UV = std::array<vec2, 4>;
    struct Location
    {
        UV uv;
        int tindex;
    };

    static constexpr int max_text_length = 480;

    size_t uv_buffer_pos;
    std::pair<int, int> next_pos;
    std::vector<TextObj> objects;

public:
    void clear()
    {
        uv_map.clear();
        /* for (auto it = uv_map.end(); it != uv_map.end();) { */
        /*     if (it->second.tindex != 0) { */
        /*         it = uv_map.erase(it); */
        /*     } else { */
        /*         it++; */
        /*     } */
        /* } */
    }

    std::unordered_map<char32_t, Location> uv_map;
    float char_width = 20;
    float char_height = 39;

    void add_char_location(char32_t c, int x, int y, int w, int h);
    void add_char_location(char32_t c, UV const&, int tindex = 0);

    int get_texture_index(char32_t c) const
    {
        auto it = uv_map.find(c);
        if (it == uv_map.end()) { return -1; }
        return it->second.tindex;
    }

    void add_text(std::pair<float, float> xy, TextAttrs const& attrs,
        std::string_view text);

    void add_text(std::pair<float, float> xy, TextAttrs const& attrs,
        std::u32string_view text32);

    void render();

    FontRenderer(int cw, int ch);
};
