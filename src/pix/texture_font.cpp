#include "texture_font.hpp"
#include <coreutils/algorithm.h>
#include <coreutils/utf8.h>
#include <fmt/format.h>
#include <gl/vec.hpp>

#include "pix.hpp"

TextureFont::TextureFont(const char* name, int size)
    : font(name, size),
      renderer(font.get_size().first, font.get_size().second),
      data(texture_width * texture_height)
{
    namespace gl = gl_wrap;

    std::tie(char_width, char_height) = font.get_size();
    fmt::print("FONT SIZE {}x{}\n", char_width, char_height);

    tile_width = char_width;
    std::fill(data.begin(), data.end(), 0);
    for (char32_t c = 0x20; c <= 0x7f; c++) {
        add_char(c);
    }

    pix::Image image{texture_width, texture_height,
        reinterpret_cast<std::byte*>(data.data()), 0};

    textures.push_back({std::make_shared<gl::Texture>(
        texture_width, texture_height, data, GL_RGBA)});
    needs_update = false;
    puts("TextureFont");
}
void TextureFont::clear()
{
    namespace gl = gl_wrap;
    next_pos = {0, 0};
    std::fill(data.begin(), data.end(), 0);
    pix::Image image{texture_width, texture_height,
        reinterpret_cast<std::byte*>(data.data()), 0};
    textures.clear();
    textures.push_back({std::make_shared<gl::Texture>(
        texture_width, texture_height, data, GL_RGBA)});
    renderer.clear();
    for (char32_t c = 0x20; c <= 0x7f; c++) {
        add_char(c);
    }
}

void TextureFont::add_char(char32_t c)
{
    if (c == 1) { return; }

    // First render character into texture
    auto* ptr = &data[next_pos.first + next_pos.second * texture_width];
    int x = next_pos.first;
    int y = next_pos.second;
    auto cw = font.render_char(c, ptr, 0xffffff00, texture_width);

    // then check if this char is wide and flag it
    if (cw < char_width) { cw = char_width; }

    float fx = static_cast<float>(x) / texture_width;
    float fy = static_cast<float>(y) / texture_height;
    float fw = static_cast<float>(cw) / texture_width;
    float fh = static_cast<float>(char_height) / texture_height;
    UV uv = {vec2{fx, fy}, {fx + fw, fy}, {fx + fw, fy + fh}, {fx, fy + fh}};
    // Add the char UV to the renderer
    // renderer.add_char_location(c, x, y, cw, char_height);
    renderer.add_tile_location(c, uv);

    next_pos.first += cw;
    if (next_pos.first >= (texture_width - char_width)) {
        next_pos.first = 0;
        next_pos.second += char_height;
    }
    needs_update = true;
}
void TextureFont::add_tile(char32_t index, gl_wrap::TexRef texture)
{
    // TODO: Usually multiple tiles are added from the same texture, so
    //       cache last and check it it's the same.
    auto it = utils::find(textures, texture);
    int tindex = 0;
    if (it == textures.end()) {
        textures.push_back(texture);
        tindex = int(textures.size() - 1);
    } else {
        tindex = int(it - textures.begin());
    }
    UV uvs = *(reinterpret_cast<UV*>(&texture.uvs));
    renderer.add_tile_location(index, uvs, tindex);
}

void TextureFont::render_text(
    std::pair<float, float> xy, TextAttrs const& attrs, std::string_view text)
{
    render_text(xy, attrs, utils::utf8_decode(text));
}

void TextureFont::render_text(std::pair<float, float> xy,
    TextAttrs const& attrs, std::u32string_view text32)
{
    int last_index = -1;
    int lasti = 0;
    int i = 0;

    auto draw = [&] {
        fflush(stdout);
        renderer.add_text(xy, {attrs.fg, attrs.bg, attrs.scale},
            text32.substr(lasti, i - lasti));
        textures[last_index].bind(0);
        renderer.render();
        xy.first += static_cast<float>((i - lasti) * tile_width);
        lasti = i;
    };

    for (char32_t c : text32) {
        // if (c == 1) { i++; continue; }

        int tindex = renderer.get_texture_index(c);
        if (tindex == -1) {
            tindex = 0;
            add_char(c);
        }

        if (last_index >= 0 && tindex != last_index) {
            render();
            if (lasti != i) { draw(); }
        }
        last_index = tindex;
        i++;
    }
    render();
    draw();
}

void TextureFont::set_tile_size(int tw, int th)
{
    renderer.set_tile_size(tw, th);
    tile_width = tw;
}

void TextureFont::render()
{
    namespace gl = gl_wrap;

    if (needs_update) {
        textures[0].tex = std::make_shared<gl::Texture>(
            texture_width, texture_height, data, GL_RGBA);
        needs_update = false;
    }
}
