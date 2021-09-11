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
    std::shared_ptr<gl_wrap::Texture> canvas;

    mrb::RubyPtr current_font;

    //std::array<double, 16> const Id = {
    //    1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    std::pair<double, double> last_point;

    pix::Image read_image(int x, int y, int w, int h);
    void draw_line(
        double x0, double y0, double x1, double y1, RStyle const* style = nullptr);
    void draw_circle(double x, double y, double r, RStyle const* style = nullptr);
    void draw_image(double x, double y, RImage* image, double scale = 1.0F,
        RStyle const* style = nullptr);
    void draw_quad(double x, double y, double w, double h, RStyle const* style = nullptr);

public:
    void init(mrb_state* mrb);
    static inline RClass* rclass;
    static inline mrb_data_type dt{"Canvas",
        [](mrb_state*,
            void* data) { /*delete static_cast<GLConsole *>(data); */ }};

    void clear();
    RCanvas(int w, int h);
    virtual ~RCanvas() = default;
    void render() override;
    void reset() override;

    static void reg_class(mrb_state* ruby);
};

