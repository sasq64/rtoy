#pragma once

#include <cstdint>
#include <string>
#include <string_view>

// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

//#define UTF8_ACCEPT 0
//#define UTF8_REJECT 1

namespace utils {

inline uint32_t decode(uint32_t* state, uint32_t* codep, char byte)
{
    static const uint8_t utf8d[] = {
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 00..1f
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 20..3f
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 40..5f
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 60..7f
        1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
        1,   1,   1,   1,   1,   9,   9,   9,   9,   9,   9,
        9,   9,   9,   9,   9,   9,   9,   9,   9,   9, // 80..9f
        7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,
        7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,
        7,   7,   7,   7,   7,   7,   7,   7,   7,   7, // a0..bf
        8,   8,   2,   2,   2,   2,   2,   2,   2,   2,   2,
        2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
        2,   2,   2,   2,   2,   2,   2,   2,   2,   2, // c0..df
        0xa, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
        0x3, 0x3, 0x4, 0x3, 0x3, // e0..ef
        0xb, 0x6, 0x6, 0x6, 0x5, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
        0x8, 0x8, 0x8, 0x8, 0x8, // f0..ff
        0x0, 0x1, 0x2, 0x3, 0x5, 0x8, 0x7, 0x1, 0x1, 0x1, 0x4,
        0x6, 0x1, 0x1, 0x1, 0x1, // s0..s0
        1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
        1,   1,   1,   1,   1,   1,   0,   1,   1,   1,   1,
        1,   0,   1,   0,   1,   1,   1,   1,   1,   1, // s1..s2
        1,   2,   1,   1,   1,   1,   1,   2,   1,   2,   1,
        1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
        1,   2,   1,   1,   1,   1,   1,   1,   1,   1, // s3..s4
        1,   2,   1,   1,   1,   1,   1,   1,   1,   2,   1,
        1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
        1,   3,   1,   3,   1,   1,   1,   1,   1,   1, // s5..s6
        1,   3,   1,   1,   1,   1,   1,   3,   1,   3,   1,
        1,   1,   1,   1,   1,   1,   3,   1,   1,   1,   1,
        1,   1,   1,   1,   1,   1,   1,   1,   1,   1, // s7..s8
    };

    uint32_t type = utf8d[(unsigned char)byte];

    *codep = (*state != 0) ? (byte & 0x3fu) | (*codep << 6)
                                     : (0xff >> type) & (byte);

    *state = utf8d[256 + *state * 16 + type];
    return *state;
}

inline size_t utf8_decode(const std::string& utf8, uint32_t* target)
{
    uint32_t codepoint;
    uint32_t state = 0;
    auto* ptr = target;

    for (auto s : utf8) {
        if (!decode(&state, &codepoint, s)) {
            if (codepoint <= 0xffff)
                *ptr++ = codepoint;
        }
    }
    return ptr - target;
}

inline std::u32string utf8_decode(std::string_view txt)
{
    std::u32string result;
    using C = std::u32string::value_type;

    uint32_t codepoint;
    uint32_t state = 0;

    for (auto s : txt) {
        if (!decode(&state, &codepoint, s)) {
                result.push_back((C)codepoint);
        }
    }
    return result;
}

/* inline std::string utf8_encode(std::string_view txt) */
/* { */
/*     std::string out; */
/*     const uint8_t* s = (uint8_t*)txt.data(); */
/*     for (size_t i = 0; i < txt.length(); i++) { */
/*         uint8_t c = s[i]; */
/*         if (c <= 0x7f) */
/*             out.push_back(c); */
/*         else { */
/*             out.push_back(0xC0 | (c >> 6)); */
/*             out.push_back(0x80 | (c & 0x3F)); */
/*         } */
/*     } */
/*     return out; */
/* } */

inline std::string utf8_encode(const std::u32string& s)
{
    std::string out;
    for (auto c : s) {
        if (c < 0x80)
            out.push_back(c & 0xff);
        else if (c < 0x800) {
            out.push_back(0xC0 | ((c >> 6) & 0x1f));
            out.push_back(0x80 | (c & 0x3f));
        } else if (c < 0x10000) {
            out.push_back(0xE0 | ((c >> 12) & 0xf));
            out.push_back(0x80 | ((c >> 6) & 0x3f));
            out.push_back(0x80 | (c & 0x3f));
        } else {
            out.push_back(0xF0 | ((c >> 18) & 0x07));
            out.push_back(0x80 | ((c >> 12) & 0x3f));
            out.push_back(0x80 | ((c >> 6) & 0x3f));
            out.push_back(0x80 | (c & 0x3f));
        }
    }
    return out;
}
} // namespace utils
