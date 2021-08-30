#include "pixel_console.hpp"
#include "gl/program_cache.hpp"
#include <algorithm>

std::string PixConsole::vertex_shader{R"gl( 
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

std::string PixConsole::fragment_shader{R"gl(
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

void PixConsole::add_char(char32_t c)
{
    auto cw = char_width + gap;
    std::vector<uint32_t> temp(char_width * char_height);
    font.render_char(c, temp.data(), 0xffffff00, char_width);

    font_texture.update(
        next_pos.first, next_pos.second, char_width, char_height, temp.data());

    auto fx = texture_width / 256;
    auto fy = texture_height / 256;
    int x = next_pos.first / fx;
    int y = next_pos.second / fy;
    char_uvs[c] = (x) | (y << 8);

    next_pos.first += (char_width + gap + 3) & 0xfffffffc;
    if (next_pos.first >= (texture_width - cw)) {
        next_pos.first = 0;
        next_pos.second += (char_height + gap + 3) & 0xfffffffc;
    }
}

std::pair<int, int> PixConsole::alloc_char(char32_t c)
{
    auto cw = char_width + gap;
    auto fx = texture_width / 256;
    auto fy = texture_height / 256;
    // First render character into texture
    auto res = next_pos;
    int x = next_pos.first / fx;
    int y = next_pos.second / fy;
    char_uvs[c] = (x) | (y << 8);

    next_pos.first += cw;
    if (next_pos.first >= (texture_width - cw)) {
        next_pos.first = 0;
        next_pos.second += (char_height + gap);
    }
    return res;
}

PixConsole::PixConsole(
    unsigned w, unsigned h, std::string const& font_file, int size)
    : font{font_file.c_str(), size}, width(w), height(h)
{
    // font.set_pixel_size(32);
    std::vector<uint32_t> data;
    data.resize(texture_width * texture_height);
    std::tie(char_width, char_height) = font.get_size();

    std::fill(data.begin(), data.end(), 0);

    font_texture = gl_wrap::Texture{texture_width, texture_height, data};
    for (char32_t c = 0x20; c <= 0x7f; c++) {
        add_char(c);
    }
    std::fill(data.begin(), data.end(), 0xff);

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
        std::pair<float, float>(
            static_cast<float>(char_width) / static_cast<float>(texture_width),
            static_cast<float>(char_height) /
                static_cast<float>(texture_height)));
    uv_texture.update(uvdata.data());

    for (auto& c : coldata) {
        c = 0x00FF00FF;
    }
    col_texture.update(coldata.data());
}

std::pair<unsigned, unsigned> PixConsole::get_char_size()
{
    return {char_width, char_height};
}

void PixConsole::set_tile_size(int w, int h)
{
    char_uvs.clear();
    next_pos = {0, 0};

    char_width = w;
    char_height = h;
    program.use();
    program.setUniform("uv_scale",
        std::pair<float, float>(
            static_cast<float>(char_width) / static_cast<float>(texture_width),
            static_cast<float>(char_height) /
                static_cast<float>(texture_height)));
    for (char32_t c = 0x20; c <= 0x7f; c++) {
        add_char(c);
    }
}

void PixConsole::reset()
{
    auto [cw, ch] = font.get_size();
    set_tile_size(cw, ch);
}

void PixConsole::set_tile_image(char32_t c, gl_wrap::TexRef tex)
{
    auto fx = texture_width / 256;
    auto fy = texture_height / 256;

    std::pair<int, int> pos{0, 0};
    auto it = char_uvs.find(c);
    if (it == char_uvs.end()) {
        pos = alloc_char(c);
    } else {
        auto cx = (it->second & 0xff) * fx;
        auto cy = (it->second >> 8) * fy;
        pos = {cx, cy};
    }
    pix::set_colors(
        std::array{1.0F, 1.0F, 1.0F, 1.0F}, std::array{0.0F, 0.0F, 0.0F, 0.0F});
    font_texture.set_target();
    gl_wrap::ProgramCache::get_instance().textured.use();
    tex.bind();
    pix::draw_quad_uvs(pos.first, texture_height - char_height - pos.second,
        char_width, char_height, tex.uvs);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

std::pair<int, int> PixConsole::text(int x, int y, std::string const& t)
{
    return text(x, y, t, 0xffffffff, 0x00000000);
}

std::pair<int, int> PixConsole::text(
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

void PixConsole::flush()
{
    uv_texture.update(uvdata.data());
    col_texture.update(coldata.data());
}

void PixConsole::put_char(int x, int y, char32_t c)
{
    auto it = char_uvs.find(c);
    if (it == char_uvs.end()) {
        add_char(c);
        it = char_uvs.find(c);
    }
    uvdata[x + width * y] = (uvdata[x + width * y] & 0xffff0000) | it->second;
}

uint32_t PixConsole::get_char(int x, int y)
{
    uint32_t uv = uvdata[x + width * y] & 0xffff;
    // TODO: Reverse lookup table ?
    auto it = std::find_if(char_uvs.begin(), char_uvs.end(),
        [&](auto&& kv) { return kv.second == uv; });
    if (it == char_uvs.end()) {
        fmt::print("{:x} not found\n", uv);
        return 0;
    }
    return it->first;
}

void PixConsole::put_color(int x, int y, uint32_t fg, uint32_t bg)
{
    auto [w0, w1] = make_col(fg, bg);
    uvdata[x + width * y] = (uvdata[x + width * y] & 0xffff) | w0;
    coldata[x + width * y] = w1;
}

void PixConsole::fill(uint32_t fg, uint32_t bg)
{
    auto [w0, w1] = make_col(fg, bg);
    w0 |= char_uvs[' '];
    for (int i = 0; i < uvdata.size(); i++) {
        uvdata[i] = w0;
        coldata[i] = w1;
    }
}

void PixConsole::fill(uint32_t bg)
{
    auto [w0, w1] = make_col(0, bg);
    w0 |= char_uvs[' '];
    for (auto& c : coldata) {
        c = (c & 0xff000000) | w1;
    }
}

void PixConsole::clear_area(
    int32_t x, int32_t y, int32_t w, int32_t h, uint32_t fg, uint32_t bg)
{
    if (w == -1) { w = width; }
    if (h == -1) { h = height; }
    auto [w0, w1] = make_col(fg, bg);
    w0 |= char_uvs[' '];
    for (int32_t yy = 0; yy < h; yy++) {
        for (int32_t xx = 0; xx < w; xx++) {
            auto offs = (xx + x) + (yy + y) * w;
            uvdata[offs] = w0;
            coldata[offs] = w1;
        }
    }
}

void PixConsole::scroll(int dy, int dx)
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

void PixConsole::set_scale(std::pair<float, float> s)
{
    scale = s;
}

void PixConsole::set_offset(std::pair<float, float> o)
{
    offset = o;
}

void PixConsole::render()
{
    glDisable(GL_BLEND);
    col_texture.bind(2);
    uv_texture.bind(1);
    font_texture.bind(0);
    program.use();
    float w = scale.first * static_cast<float>(width * char_width);
    float h = scale.second * static_cast<float>(height * char_height);
    pix::draw_quad_impl(offset.first, offset.second, w, h);
    glEnable(GL_BLEND);
}
