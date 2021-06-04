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
    float width() const {
        return image.width * (texture.uvs[4] - texture.uvs[0]);
    }
    float height() const {
        return image.height * (texture.uvs[5] - texture.uvs[1]);
    }

    float x() const {
        return image.width * texture.uvs[4];
    }

    float y() const {
        return image.height * texture.uvs[5];
    }


    RImage(pix::Image const& img) : image{img} {}
    void upload();

    gl::TexRef texture;

    void draw(float x, float y);

    static void reg_class(mrb_state* ruby);
};

