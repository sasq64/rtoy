#pragma once
#include "pix/pix.hpp"
#include "rfont.hpp"
#include "rlayer.hpp"

#include <gl/texture.hpp>

#include "mrb_tools.hpp"
#include <mruby.h>
#include <mruby/data.h>

class RImage;

class RCanvas : public RLayer
{
    gl_wrap::Texture canvas;

    mrb::RubyPtr current_font;

    std::array<float, 16> const Id = {
        1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    std::pair<float, float> last_point;

    pix::Image read_image(int x, int y, int w, int h);
    void draw_line(float x0, float y0, float x1, float y1);
    void draw_circle(float x, float y, float r);
    void draw_image(float x, float y, RImage* image);
public:
    void init(mrb_state* mrb);
    static inline RClass* rclass;
    static inline mrb_data_type dt{"Canvas",
        [](mrb_state*,
            void* data) { /*delete static_cast<GLConsole *>(data); */ }};

    void clear();
    RCanvas(int w, int h);
    void render() override;

    static void reg_class(mrb_state* ruby);
};

