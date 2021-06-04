#include "texture_font.hpp"
#include <coreutils/utf8.h>
#include <fmt/format.h>
#include <gl/vec.hpp>

#include "pix.hpp"

TextureFont::TextureFont(const char* name, int size)
    : font(name, size),
      renderer(texture_width, texture_height, font.get_size().first,
          font.get_size().second),
      data(texture_width * texture_height)
{
    namespace gl = gl_wrap;

    std::tie(char_width, char_height) = font.get_size();
    fmt::print("FONT SIZE {}x{}\n", char_width, char_height);

    std::fill(data.begin(), data.end(), 0);
    for (char32_t c = 0x20; c <= 0x7f; c++) {
        add_char(c);
    }

    pix::Image image{texture_width, texture_height,
        reinterpret_cast<std::byte*>(data.data()), 0};

    texture.tex = std::make_shared<gl::Texture>(
        texture_width, texture_height, data, GL_RGBA);
    needs_update = false;
    puts("TextureFont");
}

void TextureFont::add_char(char32_t c)
{
    if (c == 1) { return; }
    auto* ptr = &data[next_pos.first + next_pos.second * texture_width];

    int x = next_pos.first;
    int y = next_pos.second;
    auto cw = font.render_char(c, ptr, texture_width);

    if (cw < char_width) { cw = char_width; }
    if (cw > char_width * 2) {
        cw = char_width * 2;
        fmt::print("Wide {}\n", static_cast<int>(c));
        is_wide.insert(c);
    }

    renderer.add_char_location(c, x, y, cw, char_height);

    next_pos.first += cw;
    if (next_pos.first >= (texture_width - char_width)) {
        next_pos.first = 0;
        next_pos.second += char_height;
    }
    needs_update = true;
}

void TextureFont::add_text(
    std::pair<float, float> xy, TextAttrs const& attrs, std::string_view text)
{
    add_text(xy, attrs, utils::utf8_decode(text));
}

void TextureFont::add_text(std::pair<float, float> xy, TextAttrs const& attrs,
    std::u32string_view text32)
{
    for (char32_t c : text32) {
        if (c == 1) { continue; }
        if (!renderer.has_char(c)) { add_char(c); }
    }
    renderer.add_text(xy, {attrs.fg, attrs.bg, attrs.scale}, text32);
    render();
}

void TextureFont::render()
{
    namespace gl = gl_wrap;

    if (needs_update) {
        texture.tex = std::make_shared<gl::Texture>(
            texture_width, texture_height, data, GL_RGBA);
        needs_update = false;
    }

    texture.bind();
    renderer.render();
}
