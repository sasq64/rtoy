
#include "gl_console.hpp"
#include "pix.hpp"
#include "texture_font.hpp"

#include <gl/program_cache.hpp>

#include <coreutils/utf8.h>

void GLConsole::set_tile_size(int tw, int th)
{
    tile_width = tw;
    tile_height = th;
    font->set_tile_size(tw, th);
}

int GLConsole::get_width()
{
    return width;
}

int GLConsole::get_height()
{
    return height;
}

Console::Char GLConsole::get(int x, int y) const
{
    if (x < 0 || y < 0 || x >= width || y >= height) { return {}; }
    return grid[x + width * y];
}

/* Cursor GLConsole::text(int x, int y, std::string const& t) */
/* { */
/*     uint32_t fg = default_style.fg; */
/*     uint32_t bg = default_style.bg; */
/*     return text(x, y, t, fg, bg); */
/* } */

Cursor GLConsole::text(
    int x, int y, std::string const& t, uint32_t fg, uint32_t bg)
{
    //fmt::print("PRINT {},{}: '{}'\n", x, y, t);
    auto text32 = utils::utf8_decode(t);
    dirty[y] = 1;
    for (auto c : text32) {
        if (c == 10) {
            x = 0;
            y++;
            dirty[y] = 1;
            continue;
        }

        auto& t = grid[x + width * y];
        t = {c, fg, bg};
        x++;
        if (x >= width) {
            x -= width;
            y++;
            dirty[y] = 1;
        }
    }
    return {x, y};
}

void GLConsole::put_char(int x, int y, char32_t c)
{
    dirty[y] = 1;
    auto& t = grid[x + width * y];
    t = {c, t.fg, t.bg};
}

void GLConsole::put_color(int x, int y, uint32_t fg, uint32_t bg)
{
    dirty[y] = 1;
    auto& t = grid[x + width * y];
    t = {t.c, fg, bg};
}

void GLConsole::fill(uint32_t fg, uint32_t bg)
{
    std::fill(dirty.begin(), dirty.end(), 1);
    std::fill(grid.begin(), grid.end(), Char{' ', fg, bg});
}

void GLConsole::blit(int x, int y, int stride, std::vector<Char> const& source)
{
    int32_t i = 0;
    auto xx = x;
    for (auto const& c : source) {
        dirty[y] = 1;
        if (xx < width && y < height) { this->grid[xx + width * y] = c; }
        xx++;
        if (++i == stride) {
            xx = x;
            y++;
            i = 0;
        }
    }
}
void GLConsole::reset()
{
    fmt::print("RESET {} {}\n", font->char_width, font->char_height);
    set_tile_size(font->char_width, font->char_height);
}

GLConsole::GLConsole(int w, int h, Style style)
    : font(std::make_shared<TextureFont>(
          style.font.c_str(), style.font_size)),
      frame_buffer(w, h)
{
    width = w / font->char_width;
    height = h / font->char_height;

    tile_width = font->char_width;
    tile_height = font->char_height;
    dirty.resize(height);

    fflush(stdout);
    grid.resize(width * height);
    old_grid.resize(width * height);
    fill(style.fg, style.bg);
    frame_buffer.set_target();
    fflush(stdout);
    gl_wrap::clearColor({style.bg});
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLConsole::flush()
{
    uint32_t fg = 0;
    uint32_t bg = 0;
    std::u32string text;
    std::pair<float, float> pos;
    bool in_string = false;

    bool changed = false;
    frame_buffer.set_target();
    for (int32_t y = 0; y < height; y++) {
        if (dirty[y] == 0) { continue; }
        dirty[y] = 0;
        for (int32_t x = 0; x < width; x++) {
            auto& old = old_grid[x + y * width];
            auto const& tile = grid[x + y * width];
            if (in_string) {
                // If color changes we _must_ start a new string.
                // If tile is unchanged we also stop the string since we don't
                // need to continue.
                if (tile.fg != fg || tile.bg != bg || old == tile) {
                    font->render_text(pos, {fg, bg}, text);
                    in_string = false;
                }
            }
            if (!in_string) {
                if (old != tile) {
                    // Start of string
                    fg = tile.fg;
                    bg = tile.bg;
                    pos = {x * tile_width, y * tile_height};
                    text.clear();
                    in_string = true;
                    changed = true;
                }
            }
            if (in_string) { text.push_back(tile.c); }
            old = tile;
        }
        if (in_string) {
            font->render_text(pos, {fg, bg}, text);
            in_string = false;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLConsole::scroll(int dy, int dx)
{
    std::vector<Char> tiles;
    tiles.resize(width * height);

    for (int32_t y = 0; y < height; y++) {
        auto ty = y + dy;
        if (ty >= 0 && ty < height) {
            dirty[ty] = 1;
            for (int32_t x = 0; x < width; x++) {
                auto tx = x + dx;
                if (tx < width && ty < height) {
                    tiles[tx + ty * width] = grid[x + y * width];
                }
            }
        }
    }
    grid = tiles;
}
