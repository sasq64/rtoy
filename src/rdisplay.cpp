#include "rdisplay.hpp"

#include "mrb/class.hpp"
#include "mrb/get_args.hpp"
#include "mruby/array.h"
#include "mruby/value.h"

#include "rcanvas.hpp"
#include "rconsole.hpp"
#include "rimage.hpp"
#include "rsprites.hpp"

#include "error.hpp"
#include "gl/functions.hpp"
#include "mrb/mrb_tools.hpp"
#include "pix/pixel_console.hpp"

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <mruby/compile.h>
#include <pix/pix.hpp>

#ifdef __APPLE__
#    include "TargetConditionals.h"
#    if TARGET_OS_OSX
#        define OSX
#    endif
#endif

#include <chrono>
#include <span>

using namespace std::chrono_literals;
using clk = std::chrono::steady_clock;

Display::Display(mrb_state* state, System& system, Settings const& _settings)
    : RLayer(0, 0), ruby(state), settings{_settings}
{
    glm::mat4x4 m(1.0F);
    memcpy(Id.data(), glm::value_ptr(m), sizeof(float) * 16);

    window = system.init_screen(settings);
    setup();
}

void Display::setup()
{
    auto [w, h] = window->get_size();
    RLayer::width = w;
    RLayer::height = h;
    gl::setViewport({w, h});

    canvases = mrb_ary_new_capa(ruby, 4);
    sprite_fields = mrb_ary_new_capa(ruby, 4);
    auto style = Style{0xffffffff, 0x00008000, settings.console_font.string(),
        settings.font_size};
    auto font = std::make_shared<ConsoleFont>(
        settings.console_font.string(), settings.font_size);

    debug_console = std::make_shared<PixConsole>(40, 16, font);
    debug_console->reset();
    debug_console->fill(0xffffffff, 0x0000000);
    debug_console->text(0, 0, "DEBUG");
    debug_console->flush();

    std::vector<RConsole*> cptrs;
    auto pixel_console =
        std::make_shared<PixConsole>(256, 256, "data/unscii-16.ttf", 16);
    for (int i = 0; i < 4; i++) {
        auto con = std::make_shared<RConsole>(w, h, style, pixel_console);
        cptrs.push_back(con.get());
        layers.push_back(con);
        if (console == nullptr) {
            console = con;
        } else {
            con->enable(false);
        }
    }
    consoles = mrb::to_value(cptrs, ruby);

    for (int i = 0; i < 4; i++) {
        auto cnv = std::make_shared<RCanvas>(w, h);
        cnv->init(ruby);
        layers.push_back(cnv);
        if (canvas == nullptr) {
            canvas = cnv;
        } else {
            cnv->enable(false);
        }
        mrb_ary_set(ruby, canvases, i, mrb::to_value(cnv.get(), ruby));
    }

    for (int i = 0; i < 4; i++) {
        auto spr = std::make_shared<RSprites>(ruby, w, h);
        layers.push_back(spr);
        if (sprite_field == nullptr) {
            sprite_field = spr;
        } else {
            spr->enable(false);
        }
        mrb_ary_set(ruby, sprite_fields, i, mrb::to_value(spr.get(), ruby));
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool Display::begin_draw()
{
    pix::set_transform(Id);
    // draw_handler();
    call_proc(ruby, draw_handler, 0);
    return false;
}

void Display::end_draw()
{
    auto t = clk::now();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gl::clearColor({bg});
    glClear(GL_COLOR_BUFFER_BIT);

    gl::setViewport({width, height});
    glEnable(GL_SCISSOR_TEST);
    glScissor(scissor[0], scissor[1], width - scissor[2] * 2,
        height - scissor[3] * 2);

    auto rt = clk::now() - t;
    long clear_t =
        std::chrono::duration_cast<std::chrono::milliseconds>(rt).count();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    t = clk::now();
    for (auto&& layer : layers) {
        layer->render(this);
    }
    rt = clk::now() - t;
    long ms = std::chrono::duration_cast<std::chrono::milliseconds>(rt).count();

    if (debug_console) {
        debug_console->text(0, 0, fmt::format("CLEAR: {}ms  ", clear_t));
        debug_console->text(0, 1, fmt::format("RENDER: {}ms  ", ms));
        debug_console->text(0, 2, fmt::format("TWEEN: {}ms  ", bench_times[0]));
        debug_console->text(0, 3, fmt::format("DRAW: {}ms  ", bench_times[1]));
        debug_console->text(0, 4, fmt::format("SWAP: {}ms  ", swap_t));
        debug_console->text(0, 5, fmt::format("OTHER: {}ms  ", pre_t));
        debug_console->flush();
        debug_console->render(static_cast<float>(width - 320), 0, 1.0F, 1.0F);
    }
}

void Display::swap()
{
    auto t = clk::now();
    window->swap();
    auto rt = clk::now() - t;
    swap_t = std::chrono::duration_cast<std::chrono::milliseconds>(rt).count();
}

void Display::reset()
{
    bg = {0.0F, 0.0F, 0.8F, 1.0F};

    if (mouse_cursor != nullptr) {
        sprite_field->remove_sprite(mouse_cursor);
        mouse_cursor = nullptr;
    }
    RLayer::reset();
    for (auto&& layer : layers) {
        layer->reset();
        layer->enable(false);
    }
    console->enable(true);
    canvas->enable(true);
    sprite_field->enable(true);
    draw_handler.clear();
}

int32_t Display::dump(int x, int y)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    uint32_t v = 0;
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &v);
    // Little endian, as bytes means we get ABGR 32 bit
    return static_cast<int32_t>(
        ((v & 0xff) << 16) | (v & 0xff00) | ((v >> 16) & 0xff));
}

std::vector<int32_t> Display::dump(int x, int y, int w, int h)
{
    std::vector<int32_t> result;
    auto* ptr = new uint32_t[w * h];
    memset(ptr, 0xff, w * h * 4);
    y = height - (y + h);
    glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
    result.resize(w * h);
    int i = 0;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            auto v = ptr[x + (h - 1 - y) * w];
            result[i] = static_cast<int32_t>(
                ((v & 0xff) << 16) | (v & 0xff00) | ((v >> 16) & 0xff));
        }
    }
    return result;
}

void Display::reg_class(
    mrb_state* ruby, System& system, Settings const& settings)
{
    mrb::make_noinit_class<Display>(
        ruby, "Display", mrb::get_class<RLayer>(ruby));

    mrb::set_deleter<Display>(ruby, [](mrb_state* /*mrb*/, void* data) {
        auto* display = static_cast<Display*>(data);
        if (display != Display::default_display) { delete display; }
    });

    if (Display::default_display == nullptr) {
        Display::default_display = new Display(ruby, system, settings);
    } else {
        Display::default_display->ruby = ruby;
    }

    Display::default_display->disp_obj =
        mrb::Value{ruby, Display::default_display};

    mrb::add_class_method<Display>(
        ruby, "default", [] { return Display::default_display->disp_obj; });

    mrb::add_method<Display>(
        ruby, "mouse_ptr", [](Display* display, RImage* image) {
            display->mouse_cursor = display->sprite_field->add_sprite(image, 1);
        });

    mrb::add_method<Display>(
        ruby, "on_draw", [](Display* display, mrb::Block block) {
            display->draw_handler = block;
        });

    mrb::add_method<Display>(
        ruby, "console", [](Display* self) { return self->console.get(); });
    mrb::add_method<Display>(
        ruby, "canvas", [](Display* self) { return self->canvas.get(); });
    mrb::add_method<Display>(ruby, "sprite_field",
        [](Display* self) { return self->sprite_field.get(); });

    mrb::attr_reader<&Display::consoles>(ruby, "consoles");
    mrb::attr_reader<&Display::canvases>(ruby, "canvases");
    mrb::attr_reader<&Display::sprite_fields>(ruby, "sprite_fields");

    mrb::add_method<Display>(
        ruby, "reset", [](Display* self) { self->reset(); });

    mrb::add_method<Display>(ruby, "dump",
        [](Display* display, mrb_state* mrb, mrb::ArgN n, int x, int y, int w,
            int h) {
            if (n == 2) { return mrb::to_value(display->dump(x, y), mrb); }
            return mrb::to_value(display->dump(x, y, w, h), mrb);
        });

    mrb::add_method<Display>(ruby, "bench_start",
        [](Display* self, int) { self->bench_start = clk::now(); });

    mrb::add_method<Display>(ruby, "bench_end", [](Display* self, int i) {
        self->bench_times[i] =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                clk::now() - self->bench_start)
                .count();
    });

    mrb::add_method<Display>(ruby, "clear", [](Display* self) {
        for (auto&& layer : self->layers) {
            layer->clear();
        }
        self->end_draw();
    });

    mrb::attr_accessor<&Display::bg>(ruby, "bg");
}
