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
    pix::Image image;
    RImage(pix::Image const& img) : image{img} {}

    gl::Texture texture;

    void draw(float x, float y);

    static void reg_class(mrb_state* ruby);
};

