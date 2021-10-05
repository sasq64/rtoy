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

static constexpr int align(int val, int a)
{
    return (val + (a - 1)) & (~(a - 1));
}

void ConsoleFont::add_char(char32_t c)
{
    auto cw = char_width + gap;
    auto [fw, fh] = font.get_size();

    std::vector<uint32_t> temp(fw * fh * 2);
    font.render_char(c, temp.data(), 0xffffff00, fw);

    font_texture.update(next_pos.first, next_pos.second, fw, fh, temp.data());

    auto fx = texture_width / 256;
    auto fy = texture_height / 256;
    int x = next_pos.first / fx;
    int y = next_pos.second / fy;
    char_uvs[c] = (x) | (y << 8);

    next_pos.first += align(char_width + gap, 4);
    if (next_pos.first >= (texture_width - cw)) {
        next_pos.first = 0;
        next_pos.second += align(char_height + gap, 4);
    }
}

std::pair<int, int> ConsoleFont::alloc_char(char32_t c)
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

ConsoleFont::ConsoleFont(std::string const& font_file, int size)
    : font{font_file.c_str(), size}
{
    std::vector<uint32_t> data;
    data.resize(texture_width * texture_height);
    std::tie(char_width, char_height) = font.get_size();

    std::fill(data.begin(), data.end(), 0);

    font_texture = gl_wrap::Texture{texture_width, texture_height, data};
    for (char32_t c = 0x20; c <= 0x7f; c++) {
        add_char(c);
    }
    std::fill(data.begin(), data.end(), 0xff);
    font_texture.bind(0);
}

std::pair<float, float> ConsoleFont::get_uvscale() const
{
    return std::pair{
        static_cast<float>(char_width) / static_cast<float>(texture_width),
        static_cast<float>(char_height) / static_cast<float>(texture_height)};
}

PixConsole::PixConsole(int w, int h, std::string const& font_file, int size)
    : font{std::make_shared<ConsoleFont>(font_file.c_str(), size)},
      width(w),
      height(h)
{
    init();
}

PixConsole::PixConsole(int w, int h, std::shared_ptr<ConsoleFont> _font)
    : font{std::move(_font)}, width(w), height(h)
{
    init();
}

void PixConsole::init()
{

    uvdata.resize(width * height);
    std::fill(uvdata.begin(), uvdata.end(), 0);
    uv_texture = gl_wrap::Texture{width, height, uvdata};

    coldata.resize(width * height);
    std::fill(coldata.begin(), coldata.end(), 0);
    col_texture = gl_wrap::Texture{width, height, coldata};

    col_texture.bind(2);
    uv_texture.bind(1);

    program = gl_wrap::Program(gl_wrap::VertexShader{vertex_shader},
        gl_wrap::FragmentShader{fragment_shader});

    program.setUniform("in_tex", 0);
    program.setUniform("uv_tex", 1);
    program.setUniform("col_tex", 2);

    program.setUniform("console_size", std::pair<float, float>(width, height));
    program.setUniform("uv_scale", font->get_uvscale());
    uv_texture.update(uvdata.data());

    for (auto& c : coldata) {
        c = 0x00FF00FF;
    }
    col_texture.update(coldata.data());
}

std::pair<int, int> PixConsole::get_char_size()
{
    return {font->char_width, font->char_height};
}

void ConsoleFont::set_tile_size(int w, int h)
{
    char_uvs.clear();
    next_pos = {0, 0};

    std::vector<uint32_t> data;
    data.resize(texture_width * texture_height);
    std::fill(data.begin(), data.end(), 0);
    font_texture = gl_wrap::Texture{texture_width, texture_height, data};

    char_width = w;
    char_height = h;
    for (char32_t c = 0x20; c <= 0x7f; c++) {
        add_char(c);
    }
}

void PixConsole::reset()
{
    auto [cw, ch] = font->font.get_size();
    set_tile_size(cw, ch);
}

void ConsoleFont::set_tile_image(char32_t c, gl_wrap::TexRef tex)
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
    tex.yflip();

    glBlendFunc(GL_ONE, GL_ZERO);
    pix::draw_quad_uvs(pos.first, texture_height - char_height - pos.second,
        char_width, char_height, tex.uvs);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

std::pair<int, int> PixConsole::text(int x, int y, std::string const& t)
{
    return text(x, y, t, 0xffffffff, 0x00000000);
}

uint32_t ConsoleFont::get_offset(char32_t c)
{
    auto it = char_uvs.find(c);
    if (it == char_uvs.end()) {
        add_char(c);
        it = char_uvs.find(c);
    }
    return it->second;
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

        uvdata[x + width * y] = font->get_offset(c) | w0;
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
    uvdata[x + width * y] =
        (uvdata[x + width * y] & 0xffff0000) | font->get_offset(c);
}

uint32_t PixConsole::get_char(int x, int y)
{
    uint32_t uv = uvdata[x + width * y] & 0xffff;
    // TODO: Reverse lookup table ?
    auto it = std::find_if(font->char_uvs.begin(), font->char_uvs.end(),
        [&](auto&& kv) { return kv.second == uv; });
    if (it == font->char_uvs.end()) {
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
    w0 |= font->char_uvs[' '];
    for (size_t i = 0; i < uvdata.size(); i++) {
        uvdata[i] = w0;
        coldata[i] = w1;
    }
}

void PixConsole::fill(uint32_t bg)
{
    auto [w0, w1] = make_col(0, bg);
    w0 |= font->char_uvs[' '];
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
    w0 |= font->char_uvs[' '];
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

void PixConsole::render(float ox, float oy, float sx, float sy)
{
    glDisable(GL_BLEND);
    col_texture.bind(2);
    uv_texture.bind(1);
    font->font_texture.bind(0);
    program.use();
    float w = sx * static_cast<float>(width * font->char_width);
    float h = sy * static_cast<float>(height * font->char_height);
    pix::draw_quad_impl(ox, oy, w, h);
    glEnable(GL_BLEND);
}
