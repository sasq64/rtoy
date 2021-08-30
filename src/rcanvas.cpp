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

void RCanvas::draw_quad(float x, float y, float w, float h)
{
    canvas.set_target();
    gl::ProgramCache::get_instance().textured.use();
    pix::set_colors(style.fg, style.bg);
    pix::draw_quad_filled(x, y, w, h);
}

void RCanvas::draw_line(float x0, float y0, float x1, float y1)
{
    canvas.set_target();
    glLineWidth(style.line_width);
    pix::set_colors(style.fg, style.bg);
    pix::draw_line({x0, y0}, {x1, y1});
}

void RCanvas::draw_circle(float x, float y, float r)
{
    canvas.set_target();
    glLineWidth(style.line_width);
    pix::set_colors(style.fg, style.bg);
    pix::draw_circle({x, y}, r);
}

void RCanvas::draw_circle(float x, float y, float r, RStyle const& style)
{
    canvas.set_target();
    glLineWidth(style.line_width);
    pix::set_colors(style.fg, style.bg);
    pix::draw_circle({x, y}, r);
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

void RCanvas::draw_image(float x, float y, RImage* image, float scale)
{
    canvas.set_target();
    glLineWidth(style.line_width);
    pix::set_colors(style.fg, style.bg);
    image->draw(x, y, scale);
}

void RCanvas::render()
{
    if(!enabled) return;
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
            auto [x, y, w, h] = mrb::get_args<float, float, float, float>(mrb);
            rcanvas->draw_quad(x, y, w, h);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(2));
    mrb_define_method(
        ruby, RCanvas::rclass, "line",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto n = mrb_get_argc(mrb);
            auto* rcanvas = mrb::self_to<RCanvas>(self);
            if (n == 4) {
                auto [x0, y0, x1, y1] =
                    mrb::get_args<float, float, float, float>(mrb);
                if (x0 != x1 || y0 != y1) {
                    rcanvas->draw_line(x0, y0, x1, y1);
                }
                rcanvas->last_point = {x1, y1};
            } else if (n == 2) {
                auto [x1, y1] = mrb::get_args<float, float>(mrb);
                rcanvas->draw_line(rcanvas->last_point.first,
                    rcanvas->last_point.second, x1, y1);
                rcanvas->last_point = {x1, y1};
            }
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(2));
    mrb_define_method(
        ruby, RCanvas::rclass, "circle",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            

            auto n = mrb_get_argc(mrb);
            auto* rcanvas = mrb::self_to<RCanvas>(self);
            if (rcanvas->style.blend_mode == BlendMode::Add) {
                glBlendFunc(GL_ONE, GL_ONE);
            }
            if (n == 4) {
                RImage* image = nullptr;
                mrb_float x;
                mrb_float y;
                mrb_float r;
                RStyle* style = nullptr;
                mrb_get_args(mrb, "fffd", &x, &y, &r, &style, &RStyle::dt);
                rcanvas->draw_circle(x, y, r, *style);
            } else {
                auto [x, y, r] = mrb::get_args<float, float, float>(mrb);
                rcanvas->draw_circle(x, y, r);
            }
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(4));
    mrb_define_method(
        ruby, RCanvas::rclass, "text",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [x, y, text, size] =
                mrb::get_args<float, float, std::string, int>(mrb);
            auto* rcanvas = mrb::self_to<RCanvas>(self);

            auto fg = gl::Color(rcanvas->style.fg).to_rgba();
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
            auto n = mrb_get_argc(mrb);

            if (n == 3) {
                mrb_get_args(mrb, "ffd", &x, &y, &image, &RImage::dt);
            } else {
                mrb_get_args(mrb, "ffdf", &x, &y, &image, &RImage::dt, &scale);
            }
            auto* rcanvas = mrb::self_to<RCanvas>(self);
            rcanvas->draw_image(
                static_cast<float>(x), static_cast<float>(y), image, scale);
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
