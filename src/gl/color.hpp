#pragma once
#include "gl.hpp"

namespace gl_wrap {

struct Color
{
    GLfloat red = 0.F;
    GLfloat green = 0.F;
    GLfloat blue = 0.F;
    GLfloat alpha = 0.F;

    Color(uint32_t rgba) // NOLINT
    {
        red = (rgba >> 24) / 255.0F;
        green = ((rgba >> 16) & 0xff) / 255.0F; //NOLINT
        blue = ((rgba >> 8) & 0xff) / 255.0F; //NOLINT
        alpha = (rgba & 0xff) / 255.0F; //NOLINT
    }
};

} // namespace gl_wrap
