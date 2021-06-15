
#include "gl_console.hpp"
#include "pix.hpp"
#include "texture_font.hpp"

#include <gl/program_cache.hpp>

#include <coreutils/utf8.h>

#define Wide2 1

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

bool GLConsole::is_wide(char32_t c) const
{
    return font->is_wide.count(c) > 0;
}

Cursor GLConsole::text(int x, int y, std::string const& t)
{
    uint32_t fg = default_style.fg;
    uint32_t bg = default_style.bg;
    return text(x, y, t, fg, bg);
}

Cursor GLConsole::text(
    int x, int y, std::string const& t, uint32_t fg, uint32_t bg)
{
    // fmt::print("PRINT {},{}: '{}'\n", x, y, t);
    auto text32 = utils::utf8_decode(t);
    if (x > 0 && grid[x + width * y].c == Wide2) {
        grid[x - 1 + width * y] = {' ', fg, bg};
    }
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
        if (font->is_wide.count(c) > 0) {
            grid[x + width * y] = {Wide2, fg, bg};
            x++;
        }
        if (x >= width) {
            x -= width;
            y++;
            dirty[y] = 1;
        }
    }
    if (x < (width - 1) && grid[x + 1 + width * y].c == Wide2) {
        grid[x + 1 + width * y] = {' ', fg, bg};
    }
    return {x, y};
}

void GLConsole::put_char(int x, int y, char32_t c)
{
    dirty[y] = 1;
    auto& t = grid[x + width * y];
    if (x > 0 && t.c == Wide2) {
        grid[(x - 1) + width * y] = {' ', t.fg, t.bg};
    }
    if (x < (width - 1) && grid[x + 1 + width * y].c == Wide2) {
        grid[x + 1 + width * y] = {' ', t.fg, t.bg};
    }
    t = {c, t.fg, t.bg};
}

void GLConsole::put_color(int x, int y, uint32_t fg, uint32_t bg)
{
    dirty[y] = 1;
    auto& t = grid[x + width * y];
    if (t.c == Wide2) { grid[(x - 1) + width * y] = {' ', fg, bg}; }
    if (x < (width - 1) && grid[x + 1 + width * y].c == Wide2) {
        grid[x + 1 + width * y] = {' ', fg, bg};
    }
    t = {t.c, fg, bg};
}

void GLConsole::fill(uint32_t fg, uint32_t bg)
{
    for(auto& d : dirty) { d = 1; }
    for (auto& t : grid) {
        t = {' ', fg, bg};
    }
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

GLConsole::GLConsole(int w, int h, Style _default_style)
    : default_style(_default_style),
      font(std::make_shared<TextureFont>("data/unscii-16.ttf", 16)),
      frame_buffer(w, h)
{

    width = 256;  // w / font->char_width;
    height = 256; // h / font->char_height;

    tile_width = font->char_width;
    tile_height = font->char_height;
    dirty.resize(height);


    fflush(stdout);
    grid.resize(width * height);
    old_grid.resize(width * height);
    fill(default_style.fg, default_style.bg);
    frame_buffer.set_target();
    fflush(stdout);
    gl_wrap::clearColor({default_style.bg});
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
        if(dirty[y] == 0) { continue; }
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
    // if (changed) {
    // fmt::print("FLUSH!\n");
    // font->render();

    //}
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLConsole::scroll(int dy, int dx)
{
    std::vector<Char> tiles;
    tiles.resize(width * height);

    for (int32_t y = 0; y < height; y++) {
        auto ty = y + dy;
        if(ty  >= 0 && ty < height) {
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
