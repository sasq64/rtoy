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

    current_font = new RFont("data/Hack.ttf");

    canvas.set_target();
}
pix::Image RCanvas::read_image(int x, int y, int w, int h)
{
    pix::Image image{
        static_cast<unsigned int>(w), static_cast<unsigned int>(h)};
    image.ptr = (std::byte*)malloc(w * h * 4);
    image.sptr = std::shared_ptr<std::byte>(image.ptr, &free);
    image.format = GL_RGBA;
    void* ptr = (void*)image.ptr;
    memset(ptr, w * h * 4, 0xff);
    canvas.set_target();
    // glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
    return image;
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

void RCanvas::clear()
{
    canvas.set_target();
    gl::clearColor({0});
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RCanvas::draw_image(float x, float y, RImage* image)
{
    canvas.set_target();
    glLineWidth(style.line_width);
    pix::set_colors(style.fg, style.bg);
    image->draw(x, y);
}

void RCanvas::render()
{
    canvas.bind();
    auto& program = gl::ProgramCache::get_instance().textured;
    program.use();
    glEnable(GL_BLEND);
    pix::set_transform(transform);
    pix::set_colors(0xffffffff, 0);
    auto [w, h] = gl::getViewport();
    pix::draw_quad();
}

void RCanvas::reg_class(mrb_state* ruby)
{
    rclass = mrb_define_class(ruby, "Canvas", RLayer::rclass);
    MRB_SET_INSTANCE_TT(RCanvas::rclass, MRB_TT_DATA);

    mrb_define_method(
        ruby, RCanvas::rclass, "copy",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [x, y, w, h] = mrb::get_args<int, int, int, int>(mrb);
            auto* canvas = mrb::self_to<RCanvas>(self);
            auto img = canvas->read_image(x, y, w, h);
            pix::save_png(img, "copy.png");
            auto* image = new RImage(img);
            return mrb::new_data_obj(mrb, image);
        },
        MRB_ARGS_REQ(4));
    mrb_define_method(
        ruby, RCanvas::rclass, "line",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [x0, y0, x1, y1] =
                mrb::get_args<float, float, float, float>(mrb);
            auto* canvas = mrb::self_to<RCanvas>(self);
            canvas->draw_line(x0, y0, x1, y1);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(4));
    mrb_define_method(
        ruby, RCanvas::rclass, "circle",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [x, y, r] = mrb::get_args<float, float, float>(mrb);
            auto* canvas = mrb::self_to<RCanvas>(self);
            if (canvas->style.blend_mode == BlendMode::Add) {
                glBlendFunc(GL_ONE, GL_ONE);
            }
            canvas->draw_circle(x, y, r);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(4));
    mrb_define_method(
        ruby, RCanvas::rclass, "text",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [x, y, text, size] =
                mrb::get_args<float, float, std::string, int>(mrb);
            auto* canvas = mrb::self_to<RCanvas>(self);
            auto* rimage = canvas->current_font->render(text, size);
            canvas->draw_image(x, y, rimage);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(3));
    mrb_define_method(
        ruby, RCanvas::rclass, "draw",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            mrb_float x;
            mrb_float y;
            RImage* image;
            mrb_get_args(mrb, "ffd", &x, &y, &image, &RImage::dt);
            auto* canvas = mrb::self_to<RCanvas>(self);
            canvas->draw_image(x, y, image);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(3));
    mrb_define_method(
        ruby, RCanvas::rclass, "clear",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            mrb::self_to<RCanvas>(self)->clear();
            return mrb_nil_value();
        },
        MRB_ARGS_NONE());
}
