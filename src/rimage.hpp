#pragma once

#include <gl/texture.hpp>
#include <pix/pix.hpp>

struct mrb_state;

namespace gl = gl_wrap;

class RImage
{
public:
    gl::TexRef tex_ref;

    double width() const
    {
        fmt::print("{} {}\n", (void*)this, (void*)tex_ref.tex.get());
        return tex_ref.width();
    }
    double height() const { return tex_ref.height(); }
    double x() const { return tex_ref.x(); }
    double y() const { return tex_ref.y(); }

    explicit RImage(pix::Image const& image);
    RImage() = default;
    explicit RImage(gl::TexRef const& tex) : tex_ref(tex) {}

    void draw(double x, double y, double scale = 1.0);

    static void reg_class(mrb_state* ruby);
};

