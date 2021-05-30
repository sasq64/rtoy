
#include "gl_console.hpp"
#include "pix.hpp"
#include "texture_font.hpp"

#include <gl/program_cache.hpp>

#include <coreutils/utf8.h>

#define Wide2 1

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
    //fmt::print("PRINT {},{}: '{}'\n", x, y, t);
    auto text32 = utils::utf8_decode(t);
    if (x > 0 && grid[x + width * y].c == Wide2) {
        grid[x - 1 + width * y] = {' ', fg, bg};
    }
    for (auto c : text32) {
        if (c == 10) {
            x = 0;
            y++;
            continue;
        }

        auto& t = grid[x + width * y];
        t = {c, fg, bg};
        x++;
        if (font->is_wide.count(c) > 0) {
            grid[x + width * y] = {Wide2, fg, bg};
            x++;
        }
        if(x >= width) {
            x -= width;
            y++;
        }
    }
    if (x < (width - 1) && grid[x + 1 + width * y].c == Wide2) {
        grid[x + 1 + width * y] = {' ', fg, bg};
    }
    return {x, y};
}

void GLConsole::put_char(int x, int y, char32_t c)
{
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
    auto& t = grid[x + width * y];
    if (t.c == Wide2) { grid[(x - 1) + width * y] = {' ', fg, bg}; }
    if (x < (width - 1) && grid[x + 1 + width * y].c == Wide2) {
        grid[x + 1 + width * y] = {' ', fg, bg};
    }
    t = {t.c, fg, bg};
}

void GLConsole::fill(uint32_t fg, uint32_t bg)
{
    for (auto& t : grid) {
        t = {' ', fg, bg};
    }
}

void GLConsole::blit(int x, int y, int stride, std::vector<Char> const& source)
{
    int32_t i = 0;
    auto xx = x;
    for (auto const& c : source) {
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
    : width(w),
      height(h),
      default_style(_default_style),
      font(std::make_shared<TextureFont>("data/unscii-16-full.ttf")),
      frame_buffer(w * font->char_width, h * font->char_height)
{

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
    for (int32_t y = 0; y < height; y++) {
        for (int32_t x = 0; x < width; x++) {
            auto& old = old_grid[x + y * width];
            auto const& tile = grid[x + y * width];
            if (in_string) {
                // If color changes we _must_ start a new string.
                // If tile is unchanged we also stop the string since we don't
                // need to continue.
                if (tile.fg != fg || tile.bg != bg || old == tile) {
                    font->add_text(pos, {fg, bg}, text);
                    in_string = false;
                }
            }
            if (!in_string) {
                if (old != tile) {
                    // Start of string
                    fg = tile.fg;
                    bg = tile.bg;
                    pos = {x * font->char_width,
                        (height - 1 - y) * font->char_height};
                    text.clear();
                    in_string = true;
                    changed = true;
                }
            }
            if (in_string) { text.push_back(tile.c); }
            old = tile;
        }
        if (in_string) {
            font->add_text(pos, {fg, bg}, text);
            in_string = false;
        }
    }
    if (changed) {
        //fmt::print("FLUSH!\n");
        frame_buffer.set_target();
        font->render();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void GLConsole::scroll(int dy, int dx)
{
    std::vector<Char> tiles;
    tiles.resize(width * height);

    for (int32_t y = 0; y < height; y++) {
        for (int32_t x = 0; x < width; x++) {
            auto tx = x + dx;
            auto ty = y + dy;
            if (tx >= 0 && ty >= 0 && tx < width && ty < height) {
                tiles[tx + ty * width] = grid[x + y * width];
            }
        }
    }
    grid = tiles;
}
