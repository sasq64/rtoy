#pragma once
#include "rlayer.hpp"

#include "pix/pix.hpp"

#include <gl/texture.hpp>

#include <mruby.h>
#include <mruby/data.h>

class RImage;

class RCanvas : public RLayer
{
    gl_wrap::Texture canvas;
    std::array<float, 16> const Id = {
        1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    pix::Image read_image(int x, int y, int w, int h);
    void draw_line(float x0, float y0, float x1, float y1);
    void draw_circle(float x, float y, float r);
    void draw_image(float x, float y, RImage* image);
public:
    static inline RClass* rclass;
    static inline mrb_data_type dt{"Canvas",
        [](mrb_state*,
            void* data) { /*delete static_cast<GLConsole *>(data); */ }};

    void clear();
    RCanvas(int w, int h);
    void render() override;

    static void reg_class(mrb_state* ruby);
};

