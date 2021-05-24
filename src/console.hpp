#pragma once

#include <cstdint>
#include <unordered_set>
#include <string>
#include <vector>

struct Console
{
    struct Char
    {
        char32_t c = 0x20;
        uint32_t fg = 0;
        uint32_t bg = 0;

        bool operator==(Char const& other) const
        {
            return (other.c == c && other.fg == fg && other.bg == bg);
        }

        bool operator!=(Char const& other) const { return !operator==(other); }
    };

    virtual bool is_wide(char32_t c) const
    {
        static std::unordered_set<char32_t> wide{
            0x1fa78, 0x1f463, 0x1f311, 0x1f3f9, 0x1f4b0, 0x1f480, 0x274c};
        return wide.count(c) > 0;
    }

    virtual void text(std::string const& t) {};

    virtual int get_width() = 0;
    virtual int get_height() = 0;
    virtual Char get(int x, int y) const = 0;
    virtual void put_char(int x, int y, char32_t c) = 0;
    virtual void put_color(int x, int y, uint32_t fg, uint32_t bg) = 0;
    virtual void fill(uint32_t fg, uint32_t bg) = 0;
    virtual void blit(
        int x, int y, int stride, std::vector<Char> const& source) = 0;
    virtual void flush() = 0;
};

