#pragma once
#include "gl.hpp"
#include <array>

namespace gl_wrap {

struct Color
{
    GLfloat red = 0.F;
    GLfloat green = 0.F;
    GLfloat blue = 0.F;
    GLfloat alpha = 0.F;

    Color(std::array<float, 4> const& a) // NOLINT
    {
        red = a[0];
        green = a[1];
        blue = a[2];
        alpha = a[3];
    }

    uint32_t to_uint() const
    {
        return (static_cast<uint32_t>(red * 255) << 24) |
               (static_cast<uint32_t>(green * 255) << 16) |
               (static_cast<uint32_t>(blue * 255) << 8) |
               static_cast<uint32_t>(alpha * 255);
    }

    Color(uint32_t rgba) // NOLINT
    {
        red = (rgba >> 24) / 255.0F;
        green = ((rgba >> 16) & 0xff) / 255.0F; // NOLINT
        blue = ((rgba >> 8) & 0xff) / 255.0F;   // NOLINT
        alpha = (rgba & 0xff) / 255.0F;         // NOLINT
    }
};

} // namespace gl_wrap
