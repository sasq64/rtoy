#pragma once

#include <gl/buffer.hpp>
#include <gl/functions.hpp>
#include <gl/program.hpp>
#include <gl/shader.hpp>
#include <gl/vec.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

class TileRenderer
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

    static inline char const* vertex_shader =
        R"gl(#version 330
    #ifdef GL_ES
        precision mediump float;
    #endif
        in vec2 in_pos;
        in vec2 in_uv;
        uniform float scale;
        uniform vec2 screen_scale;
        uniform vec2 xypos;
        out vec2 out_uv;
        void main() {
            out_uv = in_uv;
            gl_Position = vec4((in_pos * scale + xypos) * screen_scale, 0, 1);
        }
    )gl";

    static inline char const* pixel_shader =
        R"gl(#version 300 es
    #ifdef GL_ES
        precision mediump float;
    #endif
        uniform sampler2D in_tex;
        in vec2 out_uv;
        uniform vec4 fg_color;
        uniform vec4 bg_color;
        out vec4 fragColor;
        void main() {
            vec4 col = texture(in_tex, out_uv);
            float a = col.a;
            col.a = mix(a, 1.0, bg_color.a);
            col.rgb = mix(fg_color.rgb * col.rgb, bg_color.rgb,
                          (1.0 - a) * bg_color.a);
            fragColor = col;
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
    }

    std::unordered_map<char32_t, Location> uv_map;

    void set_tile_size(float cw, float ch);

    void add_tile_location(char32_t c, UV const&, int tindex = 0);

    int get_texture_index(char32_t c) const
    {
        auto it = uv_map.find(c);
        if (it == uv_map.end()) { return -1; }
        return it->second.tindex;
    }

    void add_text(std::pair<float, float> xy, TextAttrs const& attrs,
        std::u32string_view text32);

    void render();

    TileRenderer(int cw, int ch);
};
