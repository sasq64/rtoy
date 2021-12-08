#include "rcanvas.hpp"

#include "mrb/mrb_tools.hpp"
#include "rimage.hpp"

#include <gl/gl.hpp>
#include <gl/program_cache.hpp>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <pix/pix.hpp>

#include <array>
#include <memory>

static std::string vertex_shader{R"gl(
#ifdef GL_ES
    precision mediump float;
#endif
    attribute vec2 in_pos;
    uniform mat4 in_transform;
    attribute vec2 in_uv;
    varying vec2 out_uv;
    void main() {
        //gl_Position = vec4( v.x, v.y, 0, 1 );
        out_uv = (in_transform * vec4(in_uv, 0, 1)).xy;
        gl_Position = vec4(in_pos, 0, 1);
    })gl"};

static std::string fragment_shader{R"gl(
    #ifdef GL_ES
        precision mediump float;
    #endif
        uniform vec4 in_color;
        uniform sampler2D in_tex;
        varying vec2 out_uv;
        void main() {
            gl_FragColor = texture2D(in_tex, out_uv) * in_color;
        })gl"};

RCanvas::RCanvas(int w, int h) : RLayer{w, h}
{
    program = gl_wrap::Program(gl_wrap::VertexShader{vertex_shader},
        gl_wrap::FragmentShader{fragment_shader});
}

void RCanvas::update_tx(RLayer const* parent)
{
    float d = 0.5;
    glm::mat4x4 m(1.0F);
    // Matrix operations are read bottom to top

    m = glm::translate(m, glm::vec3(d, d, 0));
    // 6. Apply rotation
    m = glm::rotate(m, rot, glm::vec3(0.0, 0.0, 1.0));

    // 5. Change center back so we rotate around middle of layer
    // m = glm::translate(m, glm::vec3(-1.0, 1.0, 0));

    // 4. Scale back to clip space (-1 -> 1)
    // m = glm::scale(m, glm::vec3(2.0 / width, 2.0 / height, 1.0));
    float t0 = parent != nullptr ? parent->trans[0] : 0.0F;
    float t1 = parent != nullptr ? parent->trans[1] : 0.0F;
    t0 = (t0 + trans[0]) / width;
    t1 = (t1 + trans[1]) / height;
    // 3. Translate
    m = glm::translate(m, glm::vec3(-t0, t1, 0));

    // 2. Scale to screen space and apply scale (origin is now to top left
    // corner).
    float s0 = parent != nullptr ? parent->scale[0] : 1.0F;
    float s1 = parent != nullptr ? parent->scale[1] : 1.0F;
    m = glm::scale(
        m, glm::vec3(1.0F / (scale[0] * s0), 1.0F / (scale[1] * s1), 1.0));

    // 1. Change center so 0,0 becomes the corner
    m = glm::translate(m, glm::vec3(-d, -d, 0));

    //  1
    //  ^   Clip space
    //  |
    //  |    0
    //  |
    //  +-------->1
    // -1
    //
    std::copy(glm::value_ptr(m), glm::value_ptr(m) + 16, transform.begin());

    auto lowerx = scissor[0];
    auto lowery = scissor[3];
    auto w = width - (scissor[0] + scissor[2]);
    auto h = height - (scissor[1] + scissor[3]);
    // fmt::print("{} {} {} {}\n", lowerx, lowery, w, h);
    glScissor(lowerx, lowery, w, h);

    //    glScissor(scissor[0] + trans[0] + t0, scissor[1] + trans[1] + t1,
    //      width + t0 - scissor[2] * 2, height + t1 - scissor[3] * 2);
}
void RCanvas::set_target()
{
    if (!canvas) {
        canvas = std::make_shared<gl::Texture>(width, height);
        canvas->set_target();
        canvas->fill(0x00000000);
    } else {
        canvas->set_target();
    }
}

pix::Image RCanvas::read_image(int x, int y, int w, int h)
{
    pix::Image image{w, h};
    image.ptr = static_cast<std::byte*>(malloc(sizeof(uint32_t) * w * h));
    image.sptr = std::shared_ptr<std::byte>(image.ptr, &free);
    image.format = GL_RGBA;
    auto* ptr = static_cast<void*>(image.ptr);
    memset(ptr, w * h * 4, 0xff);
    set_target();
    // glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
    return image;
}

void RCanvas::draw_quad(
    double x, double y, double w, double h, RStyle const* style)
{
    if (style == nullptr) { style = &current_style; }
    set_target();
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
    set_target();
    glLineWidth(style->line_width);
    pix::set_colors(style->fg, style->bg);
    pix::draw_line({x0, y0}, {x1, y1});
}

void RCanvas::draw_circle(double x, double y, double r, RStyle const* style)
{
    if (style == nullptr) { style = &current_style; }
    set_target();
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
    if (canvas == nullptr &&
        current_style.bg != std::array<float, 4>{0.F, 0, 0, 0}) {
        set_target();
    }
    if (canvas != nullptr) {
        canvas->set_target();
        gl::clearColor({current_style.bg});
        glClear(GL_COLOR_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
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
    set_target();
    glLineWidth(style->line_width);
    pix::set_colors(style->fg, style->bg);
    image->draw(x, y, scale);
}

void RCanvas::render(RLayer const* parent)
{
    if (!enabled || canvas == nullptr) { return; }
    update_tx(parent);
    canvas->bind();
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    // auto [w, h] = gl::getViewport();
    program.use();
    program.setUniform("in_transform", transform);
    program.setUniform("in_color", gl::Color(0xffffffff));
    std::array uvs{0.F, 0.F, 1.F, 0.F, 1.F, 1.F, 0.F, 1.F};
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    pix::draw_quad_uvs(uvs);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void RCanvas::init(mrb_state* mrb)
{
    auto* font = new RFont("data/Ubuntu-B.ttf");
    current_font = mrb::Value{mrb, font};
}

void RCanvas::reg_class(mrb_state* ruby)
{
    mrb::make_noinit_class<RCanvas>(
        ruby, "Canvas", mrb::get_class<RLayer>(ruby));

    mrb::set_deleter<RCanvas>(ruby, [](mrb_state*, void*) {});

    mrb::add_method<RCanvas>(
        ruby, "copy", [](RCanvas* self, int x, int y, int w, int h) {
            self->set_target();
            auto img = self->read_image(x, y, w, h);
            pix::save_png(img, "copy.png");
            return new RImage(img);
        });

    mrb::add_method<RCanvas>(
        ruby, "backing=", [](RCanvas* self, RImage* image) {
            self->canvas = image->texture.tex;
        });

    mrb::add_method<RCanvas>(ruby, "backing", [](RCanvas* self) {
        self->set_target();
        return new RImage(gl_wrap::TexRef{self->canvas});
    });

    mrb::add_method<RCanvas>(ruby, "as_image", [](RCanvas* self) {
        self->set_target();
        auto* image = new RImage(gl_wrap::TexRef{self->canvas});
        image->texture.yflip();
        return image;
    });

    mrb::add_method<RCanvas>(ruby, "rect",
        [](RCanvas* self, mrb::ArgN n, double x, double y, double w, double h,
           RStyle* style) {
            if (n < 5) { style = &self->current_style; }
            if (n < 3) {
                w = self->last_point.first;
                h = self->last_point.second;
            }
            self->draw_quad(x, y, w, h, style);
            self->last_point = {w, h};
        });

    mrb::add_method<RCanvas>(ruby, "line",
        [](RCanvas* self, mrb::ArgN n, double x0, double y0, double x1,
           double y1, RStyle* style) {
            if (n < 5) { style = &self->current_style; }
            if (n < 3) {
                x1 = self->last_point.first;
                y1 = self->last_point.second;
            }
            if (x0 != x1 || y0 != y1) {
                self->draw_line(x0, y0, x1, y1, style);
            }
            self->last_point = {x1, y1};
        });

    mrb::add_method<RCanvas>(ruby, "circle",
        [](RCanvas* self, mrb::ArgN n, double x, double y, double r,
           RStyle* style) {
            if (n < 4) { style = &self->current_style; }
            if (n == 1) {
                r = x;
                x = self->last_point.first;
                y = self->last_point.second;
            }
            self->last_point = {x, y};
            self->draw_circle(x, y, r, style);
        });

    mrb::add_method<RCanvas>(ruby, "text",
        [](RCanvas* self, mrb::ArgN n, double x, double y,
           std::string const& text, double size, RStyle* style) {
            if (n < 5) { style = &self->current_style; }
            self->last_point = {x, y};
            auto fg = gl::Color(style->fg).to_rgba();
            // TODO: Text image cache
            auto* rimage = self->current_font.as<RFont*>()->render(
                text, fg, static_cast<int>(size));
            self->draw_image(x, y, rimage);
            delete rimage;
        });

    mrb::add_method<RCanvas>(ruby, "draw",
        [](RCanvas* self, mrb::ArgN n, double x, double y, RImage* image,
           double scale) {
            if (n < 4) { scale = 1.0F; }
            self->last_point = {x, y};
            self->draw_image(x, y, image, scale);
        });

    mrb::add_method<&RCanvas::clear>(ruby, "clear");
    mrb::attr_accessor<&RCanvas::current_font>(ruby, "font");
}
