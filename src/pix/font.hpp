#pragma once

#include <fmt/format.h>

#include <cstdint>
#include <ft2build.h>
#include <string>
#include <type_traits>

#include FT_FREETYPE_H
#include FT_SIZES_H

class FTFont
{
    FT_Library library = nullptr;
    FT_Face face = nullptr;
    bool mono = false;

    std::pair<int, int> size;

public:
    FTFont(const char* name) // NOLINT
    {
        auto error = FT_Init_FreeType(&library);
        error = FT_New_Face(library, name, 0, &face);

        FT_Set_Pixel_Sizes(face, 8 * 2, 0);
        // FT_Set_Char_Size(face, 0, 16 * 64, 100, 100);
        if (FT_Load_Char(face, 'X', FT_LOAD_NO_BITMAP) == 0) {

            auto m = face->size->metrics;
            auto m2 = face->glyph->metrics;
            size = {m2.horiAdvance >> 6, m.height >> 6};
            fmt::print("{}x{}\n", m2.horiAdvance >> 6, m.height >> 6);
            size = {8 * 1, 8 * 2};
        }
        fmt::print("FONT {} {} {} {} {} {}\n", face->ascender, face->descender,
            face->bbox.xMin, face->bbox.xMax, face->bbox.yMin, face->bbox.yMax);
    }

    std::pair<int, int> get_size() const { return size; }

    void set_pixel_size(int w, int h) { FT_Set_Pixel_Sizes(face, w, h); }

    template <typename T>
    void copy_char(T* target, FT_Bitmap const& b, int xoffs, int yoffs,
        int stride, int width = -1, int height = -1)
    {
        auto* data = b.buffer;
        for (int y = 0; y < b.rows; y++) {
            for (int x = 0; x < b.width; x++) {
                int pitch = b.pitch;
                auto* row = &data[pitch * y];
                uint8_t alpha =
                    mono ? ((row[x >> 3] & (128 >> (x & 7))) ? 0xff : 0)
                         : data[x + y * b.pitch];
                if (x + xoffs < 0 || y + yoffs < 0) { continue; }
                if (width > 0 && x + xoffs >= width) { continue; }
                if (height > 0 && y + yoffs >= height) { continue; }
                auto offset = (x + xoffs) + (y + yoffs) * stride;
                // fmt::print("{} {} -> {}\n", x + xoffs, y + yoffs,
                // offset);
                if constexpr (sizeof(T) == 1) {
                    target[offset] = alpha;
                } else {
                    target[offset] =
                        alpha << 24 | alpha << 16 | alpha << 8 | alpha;
                }
            }
        }
    }

    template <typename T>
    std::pair<int, int> render_text(std::string_view txt, T* target, int stride,
        int width = 0, int height = 0)
    {
        int pen_x = 0;
        int pen_y = 0;
        int w = 0;
        int h = 0;

     //   set_pixel_size(0, 64);

        auto delta = face->size->metrics.ascender / 64;
        auto low = face->size->metrics.descender / 64;
        //fmt::print("{}->{}\n", delta, low);
        if(target != nullptr) {
            memset(target, 0, width*height*4);
        }
        for (auto c : txt) {
            /* load glyph image into the slot (erase previous one) */
            auto error = FT_Load_Char(face, c, FT_LOAD_RENDER);
            FT_GlyphSlot slot = face->glyph;
            if (error) continue; /* ignore errors */
            //fmt::print("{}x{} pixels to y={}\n", slot->bitmap.width, slot->bitmap.rows, delta - face->glyph->bitmap_top);
            if (target) {
                copy_char(target, slot->bitmap, pen_x + slot->bitmap_left,
                    delta - face->glyph->bitmap_top, stride);
            }
            pen_x += slot->advance.x >> 6;
        }
        return {pen_x, delta - low};
    }

    std::pair<int, int> text_size(std::string_view txt)
    {
        return render_text(txt, (uint8_t*)nullptr, 0);
    }

    template <typename T>
    int render_char(
        char32_t c, T* target, int stride, int width = 0, int height = 0)
    {
        using namespace std::string_literals;
        mono = true;
        if (FT_Load_Char(face, c,
                FT_LOAD_RENDER | (mono ? FT_LOAD_MONOCHROME : 0)) != 0) {
            // writefln("Char %x not found in font", c);
            return 0;
        }
        auto b = face->glyph->bitmap;

        auto delta = face->size->metrics.ascender / 64;

        auto xoffs = face->glyph->bitmap_left;

        auto yoffs = delta - face->glyph->bitmap_top;

        /* fmt::print("'{}' ({},{}) + {}x{}  {},{}\n"s, (char)c, xoffs,
         * yoffs,
         */
        /*     b.width, b.rows, face->glyph->advance.x, */
        /*     face->glyph->metrics.width); */

        copy_char(target, b, xoffs, yoffs, stride, width, height);

        return xoffs + static_cast<int>(b.width);
    }
};

