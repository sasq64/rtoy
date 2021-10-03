#pragma once

#include <gl/texture.hpp>
#include <mruby.h>
#include <mruby/data.h>
#include <pix/pix.hpp>

struct RClass;

namespace gl = gl_wrap;

class RImage
{
public:
    static inline RClass* rclass = nullptr;
    static mrb_data_type dt;
    gl::TexRef texture;

    double width() const { return texture.width(); }
    double height() const { return texture.height(); }
    double x() const { return texture.x(); }
    double y() const { return texture.y(); }

    explicit RImage(pix::Image const& img);
    explicit RImage(gl::TexRef const& tex) : texture(tex) {}

    void draw(double x, double y, double scale = 1.0);

    static void reg_class(mrb_state* ruby);
};

