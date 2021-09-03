#include "rcanvas.hpp"

#include "mrb_tools.hpp"
#include "rimage.hpp"

#include <gl/gl.hpp>
#include <gl/program_cache.hpp>
#include <pix/pix.hpp>

#include <mruby/class.h>

#include <array>
#include <memory>

RCanvas::RCanvas(int w, int h) : RLayer{w, h}
{
    canvas = gl::Texture(w, h);
    canvas.set_target();
    gl::clearColor({0x00ff0000});
    glClear(GL_COLOR_BUFFER_BIT);

    canvas.set_target();
}
pix::Image RCanvas::read_image(int x, int y, int w, int h)
{
    pix::Image image{
        static_cast<unsigned int>(w), static_cast<unsigned int>(h)};
    image.ptr = static_cast<std::byte*>(malloc(w * h * 4));
    image.sptr = std::shared_ptr<std::byte>(image.ptr, &free);
    image.format = GL_RGBA;
    auto* ptr = static_cast<void*>(image.ptr);
    memset(ptr, w * h * 4, 0xff);
    canvas.set_target();
    // glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
    return image;
}

void RCanvas::draw_quad(double x, double y, double w, double h, RStyle const* style)
{
    if (style == nullptr) { style = &current_style; }
    canvas.set_target();
    gl::ProgramCache::get_instance().textured.use();
    pix::set_colors(style->fg, style->bg);
    if (style->blend_mode == BlendMode::Add) { glBlendFunc(GL_ONE, GL_ONE); }
    pix::draw_quad_filled(x, y, w, h);
    if (style->blend_mode == BlendMode::Add) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
}

void RCanvas::draw_line(
    double x0, double y0, double x1, double y1, RStyle const* style)
{
    if (style == nullptr) { style = &current_style; }
    canvas.set_target();
    glLineWidth(style->line_width);
    pix::set_colors(style->fg, style->bg);
    pix::draw_line({x0, y0}, {x1, y1});
}

void RCanvas::draw_circle(double x, double y, double r, RStyle const* style)
{
    if (style == nullptr) { style = &current_style; }
    canvas.set_target();
    glLineWidth(style->line_width);
    pix::set_colors(style->fg, style->bg);
    if (style->blend_mode == BlendMode::Add) { glBlendFunc(GL_ONE, GL_ONE); }
    pix::draw_circle({x, y}, r);
    if (style->blend_mode == BlendMode::Add) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
}

void RCanvas::clear()
{
    canvas.set_target();
    gl::clearColor({0});
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RCanvas::reset()
{
    RLayer::reset();
    clear();
}

void RCanvas::draw_image(
    double x, double y, RImage* image, double scale, RStyle const* style)
{
    if (style == nullptr) { style = &current_style; }
    canvas.set_target();
    glLineWidth(current_style.line_width);
    pix::set_colors(style->fg, style->bg);
    image->draw(x, y, scale);
}

void RCanvas::render()
{
    if (!enabled) { return;
}
    canvas.bind();
    glEnable(GL_BLEND);
    pix::set_transform(transform);
    pix::set_colors(0xffffffff, 0);
    auto [w, h] = gl::getViewport();
    auto& program = gl::ProgramCache::get_instance().textured;
    program.use();
    pix::draw_quad();
}

void RCanvas::init(mrb_state* mrb)
{
    auto* font = new RFont("data/Ubuntu-B.ttf");
    current_font = mrb::RubyPtr{mrb, mrb::new_data_obj(mrb, font)};
}

void RCanvas::reg_class(mrb_state* ruby)
{
    rclass = mrb_define_class(ruby, "Canvas", RLayer::rclass);
    MRB_SET_INSTANCE_TT(RCanvas::rclass, MRB_TT_DATA);

    mrb_define_method(
        ruby, RCanvas::rclass, "copy",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [x, y, w, h] = mrb::get_args<int, int, int, int>(mrb);
            auto* rcanvas = mrb::self_to<RCanvas>(self);
            auto img = rcanvas->read_image(x, y, w, h);
            pix::save_png(img, "copy.png");
            auto* image = new RImage(img);
            return mrb::new_data_obj(mrb, image);
        },
        MRB_ARGS_REQ(4));

    mrb_define_method(
        ruby, RCanvas::rclass, "rect",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto n = mrb_get_argc(mrb);
            auto* rcanvas = mrb::self_to<RCanvas>(self);
            mrb_float w = rcanvas->last_point.first;
            mrb_float h = rcanvas->last_point.second;
            mrb_float x = 0;
            mrb_float y = 0;
            RStyle* style = &rcanvas->current_style;
            if (n == 5) {
                mrb_get_args(mrb, "ffffd", &x, &y, &w, &h, &style, &RStyle::dt);
            } else if (n == 4) {
                mrb_get_args(mrb, "ffff", &x, &y, &w, &h);
            } else if (n == 2) {
                mrb_get_args(mrb, "ff", &x, &y);
            }
            rcanvas->draw_quad(x, y, w, h, style);
            rcanvas->last_point = {w, h};
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(2));

    mrb_define_method(
        ruby, RCanvas::rclass, "line",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto n = mrb_get_argc(mrb);
            auto* rcanvas = mrb::self_to<RCanvas>(self);
            mrb_float x0 = rcanvas->last_point.first;
            mrb_float y0 = rcanvas->last_point.second;
            mrb_float x1 = 0;
            mrb_float y1 = 0;
            RStyle* style = &rcanvas->current_style;
            if (n == 5) {
                mrb_get_args(
                    mrb, "ffffd", &x0, &y0, &x1, &y1, &style, &RStyle::dt);
            } else if (n == 4) {
                mrb_get_args(mrb, "ffff", &x0, &y0, &x1, &y1);
            } else if (n == 2) {
                mrb_get_args(mrb, "ff", &x1, &y1);
            }
            if (x0 != x1 || y0 != y1) {
                rcanvas->draw_line(x0, y0, x1, y1, style);
            }
            rcanvas->last_point = {x1, y1};
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(2));

    mrb_define_method(
        ruby, RCanvas::rclass, "circle",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto n = mrb_get_argc(mrb);
            auto* rcanvas = mrb::self_to<RCanvas>(self);

            mrb_float x = rcanvas->last_point.first;
            mrb_float y = rcanvas->last_point.second;
            mrb_float r = 0.0;
            RStyle* style = &rcanvas->current_style;

            if (n == 4) {
                mrb_get_args(mrb, "fffd", &x, &y, &r, &style, &RStyle::dt);
            } else if (n == 3) {
                mrb_get_args(mrb, "fff", &x, &y, &r);
            } else {
                mrb_get_args(mrb, "f", &r);
            }
            rcanvas->last_point = {x, y};
            rcanvas->draw_circle(x, y, r, style);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(4));

    mrb_define_method(
        ruby, RCanvas::rclass, "text",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [x, y, text, size] =
                mrb::get_args<double, double, std::string, int>(mrb);
            auto* rcanvas = mrb::self_to<RCanvas>(self);

            auto fg = gl::Color(rcanvas->current_style.fg).to_rgba();
            auto* rimage =
                rcanvas->current_font.as<RFont>()->render(text, fg, size);
            rcanvas->draw_image(x, y, rimage);
            delete rimage;
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(3));

    mrb_define_method(
        ruby, RCanvas::rclass, "draw",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            mrb_float x{};
            mrb_float y{};
            mrb_float scale = 1.0F;
            RImage* image{};
            auto* rcanvas = mrb::self_to<RCanvas>(self);
            RStyle* style = &rcanvas->current_style;
            auto n = mrb_get_argc(mrb);

            if (n == 3) {
                mrb_get_args(mrb, "ffd", &x, &y, &image, &RImage::dt);
            } else if (n == 4) {
                mrb_get_args(mrb, "ffdf", &x, &y, &image, &RImage::dt, &scale);
            } else if (n == 5) {
                mrb_get_args(mrb, "ffdfd", &x, &y, &image, &RImage::dt, &scale, &style, &RStyle::dt);
            }
            rcanvas->draw_image(
                static_cast<double>(x), static_cast<double>(y), image, scale);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(3));
    mrb_define_method(
        ruby, RCanvas::rclass, "clear",
        [](mrb_state* /*mrb*/, mrb_value self) -> mrb_value {
            mrb::self_to<RCanvas>(self)->clear();
            return mrb_nil_value();
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, RCanvas::rclass, "font=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* rcanvas = mrb::self_to<RCanvas>(self);
            auto [font] = mrb::get_args<mrb_value>(mrb);
            rcanvas->current_font = mrb::RubyPtr{mrb, font};
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));
    mrb_define_method(
        ruby, RCanvas::rclass, "font",
        [](mrb_state* /*mrb*/, mrb_value self) -> mrb_value {
            auto* rcanvas = mrb::self_to<RCanvas>(self);
            return rcanvas->current_font;
        },
        MRB_ARGS_NONE());
}
