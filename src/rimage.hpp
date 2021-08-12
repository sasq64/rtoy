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
    float width() const
    {
        return static_cast<float>(image.width) *
               (texture.uvs[4] - texture.uvs[0]);
    }
    float height() const
    {
        return static_cast<float>(image.height) *
               (texture.uvs[5] - texture.uvs[1]);
    }

    float x() const { return static_cast<float>(image.width) * texture.uvs[0]; }

    float y() const
    {
        return static_cast<float>(image.height) * texture.uvs[1];
    }

    explicit RImage(pix::Image const& img) : image{img} {}
    void upload();

    gl::TexRef texture;

    void draw(float x, float y, float scale = 1.0F);

    static void reg_class(mrb_state* ruby);
};

