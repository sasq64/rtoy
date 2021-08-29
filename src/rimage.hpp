#pragma once

#include <gl/texture.hpp>
#include <mruby.h>
#include <mruby/data.h>
#include <pix/pix.hpp>

struct RClass;

namespace gl = gl_wrap;

struct RImage
{
    static inline RClass* rclass = nullptr;
    static mrb_data_type dt;
    gl::TexRef texture;

    int width() const { return texture.width(); }
    int height() const { return texture.height(); }
    int x() const { return texture.x(); }
    int y() const { return texture.y(); }

    explicit RImage(pix::Image const& img) { upload(img); }
    explicit RImage(gl::TexRef const& tex) : texture(tex) {}

    void upload(pix::Image const& img);

    void draw(float x, float y, float scale = 1.0F);

    static void reg_class(mrb_state* ruby);
};

