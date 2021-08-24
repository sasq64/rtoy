#pragma once

#include <gl/color.hpp>
#include <gl/functions.hpp>
#include <gl/gl.hpp>
#include <gl/program.hpp>
#include <gl/texture.hpp>
#include <pix/font.hpp>

#include <pix/pix.hpp>

#include <unordered_map>

class PixConsole
{
    unsigned texture_width = 512;
    unsigned texture_height = 512;
    unsigned char_width = 13;
    unsigned char_height = 24;
    bool dirty = false;

    FTFont font;

    std::string vertex_shader{R"gl( 
    #ifdef GL_ES
        precision mediump float;
    #endif
        attribute vec2 in_pos;
        attribute vec2 in_uv;
        varying vec2 out_uv;
        void main() {
            vec4 v = vec4(in_pos, 0, 1);
            gl_Position = vec4( v.x, v.y, 0, 1 );
            out_uv = in_uv;
        })gl"};

    std::string fragment_shader{R"gl(
    #ifdef GL_ES
        precision mediump float;
    #endif
        uniform sampler2D in_tex;
        uniform sampler2D uv_tex;
        uniform sampler2D col_tex;

        uniform vec2 console_size;
        uniform vec2 uv_scale;
        varying vec2 out_uv;

        void main() {
              vec4 up = texture2D(uv_tex, out_uv);
              vec4 color = texture2D(col_tex, out_uv);
              vec4 fg_color = vec4(up.wz, color.a, 1.0);
              vec4 bg_color = vec4(color.rgb, 1.0);
              vec2 ux = (up.xy * 255.0) / 256.0;
              vec2 uvf = fract(out_uv * console_size);
              vec2 uv = ux + uvf * uv_scale;
              vec4 col = texture2D(in_tex, uv);

              gl_FragColor = fg_color * col * col.a + bg_color * (1.0 - col.a);
        })gl"};

    std::pair<int, int> next_pos{0, 0};

    std::unordered_map<char32_t, uint32_t> char_uvs;
    gl_wrap::Texture font_texture;
    gl_wrap::Texture uv_texture;
    gl_wrap::Texture col_texture;
    gl_wrap::Program program;
    std::vector<uint32_t> data;
    std::vector<uint32_t> uvdata;
    std::vector<uint32_t> coldata;

    unsigned width;
    unsigned height;

    static constexpr std::pair<uint32_t, uint32_t> make_col(
        uint32_t fg, uint32_t bg)
    {
        bg >>= 8;
        return std::pair<uint32_t, uint32_t>{
            fg & 0xffff0000, ((bg & 0xff) << 16) | (bg & 0xff00) | (bg >> 16) |
                                 ((fg << 16) & 0xff000000)};
    }

    void add_char(char32_t c)
    {
        auto cw = char_width + 2;
        // First render character into texture
        auto* ptr = &data[next_pos.first + next_pos.second * texture_width];
        int x = next_pos.first / 2;
        int y = next_pos.second / 2;
        font.render_char(c, ptr, 0xffffff00, texture_width);
        char_uvs[c] = (x) | (y << 8);

        next_pos.first += cw;
        if (next_pos.first >= (texture_width - cw)) {
            next_pos.first = 0;
            next_pos.second += (char_height + 2);
        }
    }

    uint32_t* alloc_char(char32_t c)
    {
        auto cw = char_width + 2;
        // First render character into texture
        auto* ptr = &data[next_pos.first + next_pos.second * texture_width];
        int x = next_pos.first / 2;
        int y = next_pos.second / 2;
        char_uvs[c] = (x) | (y << 8);

        next_pos.first += cw;
        if (next_pos.first >= (texture_width - cw)) {
            next_pos.first = 0;
            next_pos.second += (char_height + 2);
        }
        return ptr;
    }

public:
    PixConsole(unsigned w, unsigned h,
        std::string const& font_file = "data/bedstead.otf", int size = 32)
        : font{font_file.c_str(), size}, width(w), height(h)
    {
        // font.set_pixel_size(32);
        data.resize(texture_width * texture_height);
        std::tie(char_width, char_height) = font.get_size();

        char_height = (char_height + 1) & 0xfffffffe;
        char_width = (char_width + 1) & 0xfffffffe;

        std::fill(data.begin(), data.end(), 0);

        for (char32_t c = 0x20; c <= 0x7f; c++) {
            add_char(c);
        }
        font_texture = gl_wrap::Texture{texture_width, texture_height, data};

        uvdata.resize(w * h);
        std::fill(uvdata.begin(), uvdata.end(), 0);
        uv_texture = gl_wrap::Texture{w, h, uvdata};

        coldata.resize(w * h);
        std::fill(coldata.begin(), coldata.end(), 0);
        col_texture = gl_wrap::Texture{w, h, coldata};

        col_texture.bind(2);
        uv_texture.bind(1);
        font_texture.bind(0);

        program = gl_wrap::Program({vertex_shader}, {fragment_shader});

        program.setUniform("in_tex", 0);
        program.setUniform("uv_tex", 1);
        program.setUniform("col_tex", 2);

        program.setUniform("console_size", std::pair<float, float>(w, h));
        program.setUniform("uv_scale",
            std::pair<float, float>(static_cast<float>(char_width) /
                                        static_cast<float>(texture_width),
                static_cast<float>(char_height) /
                    static_cast<float>(texture_height)));
        uv_texture.update(uvdata.data());

        for (auto& c : coldata) {
            c = 0x00FF00FF;
        }
        col_texture.update(coldata.data());
    }

    std::pair<unsigned, unsigned> get_char_size()
    {
        return {char_width, char_height};
    }

    void set_tile_size(int w, int h)
    {
        char_uvs.clear();
        next_pos = {0, 0};

        char_width = (w + 1) & 0xfffffffe;
        char_height = (h + 1) & 0xfffffffe;
        program.use();
        program.setUniform("uv_scale",
            std::pair<float, float>(static_cast<float>(char_width) /
                                        static_cast<float>(texture_width),
                static_cast<float>(char_height) /
                    static_cast<float>(texture_height)));
        std::fill(data.begin(), data.end(), 0);
        for (char32_t c = 0x20; c <= 0x7f; c++) {
            add_char(c);
        }
        font_texture = gl_wrap::Texture{texture_width, texture_height, data};
    }

    void set_tile_image(char32_t c, pix::Image const& image, int x = 0,
        int y = 0, int w = -1, int h = -1)
    {
        if (w < 0) w = image.width;
        if (h < 0) h = image.height;

        uint32_t* ptr = nullptr;
        auto it = char_uvs.find(c);
        if (it == char_uvs.end()) {
            auto* ptr = alloc_char(c);
        } else {
            auto cx = (it->second & 0xff) * 2;
            auto cy = (it->second >> 8) * 2;
            ptr = &data[cx + cy * texture_width];
        }

        uint32_t* src =
            reinterpret_cast<uint32_t*>(image.ptr) + x + y * image.width;
        while (h > 0) {
            auto width = w;
            while (width > 0) {
                *ptr++ = *src++;
                width--;
            }
            ptr += (texture_width - w);
            src += (image.width - w);
            h--;
        }
        font_texture = gl_wrap::Texture{texture_width, texture_height, data};
    }

    std::pair<int, int> text(int x, int y, std::string const& t)
    {
        return text(x, y, t, 0xffffffff, 0x00000000);
    }

    std::pair<int, int> text(
        int x, int y, std::string const& t, uint32_t fg, uint32_t bg)
    {
        auto text32 = utils::utf8_decode(t);

        auto [w0, w1] = make_col(fg, bg);
        for (auto c : text32) {
            if (c == 10) {
                x = 0;
                y++;
                continue;
            }

            auto it = char_uvs.find(c);
            if (it == char_uvs.end()) {
                add_char(c);
                it = char_uvs.find(c);
                dirty = true;
            }

            uvdata[x + width * y] = it->second | w0;
            coldata[x + width * y] = w1;
            x++;
            if (x >= width) {
                x = 0;
                y++;
            }
        }
        return {x, y};
    }

    void flush()
    {
        if (dirty) {
            font_texture.update(data.data());
            dirty = false;
        }
        uv_texture.update(uvdata.data());
        col_texture.update(coldata.data());
    }

    void put_char(int x, int y, char32_t c)
    {
        auto it = char_uvs.find(c);
        if (it == char_uvs.end()) {
            add_char(c);
            it = char_uvs.find(c);
        }
        uvdata[x + width * y] =
            (uvdata[x + width * y] & 0xffff0000) | it->second;
    }

    uint32_t get_char(int x, int y)
    {
        uint32_t uv = uvdata[x + width * y] & 0xffff;
        // TODO: Reverse lookup table ?
        auto it = std::find_if(char_uvs.begin(), char_uvs.end(),
            [&](auto&& kv) { return kv.second == uv; });
        return it->first;
    }

    void put_color(int x, int y, uint32_t fg, uint32_t bg)
    {
        auto [w0, w1] = make_col(fg, bg);
        uvdata[x + width * y] = (uvdata[x + width * y] & 0xffff) | w0;
        coldata[x + width * y] = w1;
    }

    void fill(uint32_t fg, uint32_t bg)
    {
        auto [w0, w1] = make_col(fg, bg);
        w0 |= char_uvs[' '];
        for (int i = 0; i < uvdata.size(); i++) {
            uvdata[i] = w0;
            coldata[i] = w1;
        }
    }

    void fill(uint32_t bg)
    {
        auto [w0, w1] = make_col(0, bg);
        w0 |= char_uvs[' '];
        for (auto& c : coldata) {
            c = (c & 0xff000000) | w1;
        }
    }

    void scroll(int dy, int dx)
    {
        // TODO: Optimize
        auto uc = uvdata;
        auto cc = coldata;
        for (int32_t y = 0; y < height; y++) {
            auto ty = y + dy;
            if (ty >= 0 && ty < height) {
                for (int32_t x = 0; x < width; x++) {
                    auto tx = x + dx;
                    if (tx < width && ty < height) {
                        uvdata[tx + ty * width] = uc[x + y * width];
                        coldata[tx + ty * width] = cc[x + y * width];
                    }
                }
            }
        }
    }

    std::pair<float, float> scale{2.0, 2.0};
    std::pair<float, float> offset{0, 0};

    void set_scale(std::pair<float, float> s) { scale = s; }

    void set_offset(std::pair<float, float> o) { offset = o; }

    void render()
    {
        glDisable(GL_BLEND);
        col_texture.bind(2);
        uv_texture.bind(1);
        font_texture.bind(0);
        program.use();
        float w = scale.first * static_cast<float>(width * char_width);
        float h = scale.second * static_cast<float>(height * char_height);
        pix::draw_quad_impl(offset.first, offset.second, w, h);
    }
};
