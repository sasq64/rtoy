#pragma once

#include <gl/texture.hpp>
#include <pix/pix.hpp>

struct mrb_state;

namespace gl = gl_wrap;

class RImage
{
public:
    gl::TexRef texture;

    double width() const
    {
        fmt::print("{} {}\n", (void*)this, (void*)texture.tex.get());
        return texture.width();
    }
    double height() const { return texture.height(); }
    double x() const { return texture.x(); }
    double y() const { return texture.y(); }

    RImage() = default;
    explicit RImage(pix::Image const& img);
    explicit RImage(gl::TexRef const& tex) : texture(tex) {}

    void draw(double x, double y, double scale = 1.0);

    static void reg_class(mrb_state* ruby);
};

