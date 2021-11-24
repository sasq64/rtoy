#include "rdisplay.hpp"

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

    consoles = mrb_ary_new_capa(ruby, 4);
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

    auto pixel_console =
        std::make_shared<PixConsole>(256, 256, "data/unscii-16.ttf", 16);
    for (int i = 0; i < 4; i++) {
        auto con = std::make_shared<RConsole>(w, h, style, pixel_console);
        layers.push_back(con);
        if (console == nullptr) {
            console = con;
        } else {
            con->enable(false);
        }
        mrb_ary_set(ruby, consoles, i, mrb::new_data_obj(ruby, con.get()));
    }

    for (int i = 0; i < 4; i++) {
        auto cnv = std::make_shared<RCanvas>(w, h);
        cnv->init(ruby);
        layers.push_back(cnv);
        if (canvas == nullptr) {
            canvas = cnv;
        } else {
            cnv->enable(false);
        }
        mrb_ary_set(ruby, canvases, i, mrb::new_data_obj(ruby, cnv.get()));
    }

    for (int i = 0; i < 4; i++) {
        auto spr = std::make_shared<RSprites>(ruby, w, h);
        layers.push_back(spr);
        if (sprite_field == nullptr) {
            sprite_field = spr;
        } else {
            spr->enable(false);
        }
        mrb_ary_set(ruby, sprite_fields, i, mrb::new_data_obj(ruby, spr.get()));
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool Display::begin_draw()
{
    pix::set_transform(Id);
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

    SET_NIL_VALUE(draw_handler);
}

void Display::reg_class(
    mrb_state* ruby, System& system, Settings const& settings)
{
    Display::rclass = mrb::make_noinit_class<Display>(
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
        mrb::new_data_obj(ruby, Display::default_display);
    mrb_gc_register(ruby, Display::default_display->disp_obj);

    mrb::add_class_method<Display>(ruby, "default", [] {
        return Display::default_display->disp_obj;
    });

//    mrb_define_class_method(
//        ruby, Display::rclass, "default",
//        [](mrb_state* /*mrb*/, mrb_value /*self*/) -> mrb_value {
//            return Display::default_display->disp_obj;
//        },
//        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, Display::rclass, "mouse_ptr",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* display = mrb::self_to<Display>(self);
            RImage* image = nullptr;
            mrb_get_args(mrb, "d", &image, mrb::get_data_type<RImage>(mrb));
            display->mouse_cursor = display->sprite_field->add_sprite(image, 1);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(3));

    mrb::add_method<Display>(ruby, "on_draw",
        [](Display* display, mrb_state* mrb) -> mrb_value {
            mrb_get_args(mrb, "&!", &display->draw_handler);
            mrb_gc_register(mrb, display->draw_handler);
            return mrb_nil_value();
        });
//        MRB_ARGS_BLOCK());

    mrb_define_method(
        ruby, Display::rclass, "console",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* display = mrb::self_to<Display>(self);
            return mrb::new_data_obj(mrb, display->console.get());
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, Display::rclass, "consoles",
        [](mrb_state* /*mrb*/, mrb_value self) -> mrb_value {
            auto* display = mrb::self_to<Display>(self);
            return display->consoles;
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, Display::rclass, "canvas",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* display = mrb::self_to<Display>(self);
            return mrb::new_data_obj(mrb, display->canvas.get());
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, Display::rclass, "canvases",
        [](mrb_state* /*mrb*/, mrb_value self) -> mrb_value {
            auto* display = mrb::self_to<Display>(self);
            return display->canvases;
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, Display::rclass, "sprite_field",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* display = mrb::self_to<Display>(self);
            return mrb::new_data_obj(mrb, display->sprite_field.get());
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, Display::rclass, "sprite_fields",
        [](mrb_state* /*mrb*/, mrb_value self) -> mrb_value {
            auto* display = mrb::self_to<Display>(self);
            return display->sprite_fields;
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, Display::rclass, "reset",
        [](mrb_state* /*mrb*/, mrb_value self) -> mrb_value {
            auto* display = mrb::self_to<Display>(self);
            display->reset();
            return mrb_nil_value();
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, Display::rclass, "dump",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* display = mrb::self_to<Display>(self);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            if (mrb_get_argc(mrb) == 2) {
                auto [x, y] = mrb::get_args<int, int>(mrb);
                uint32_t v = 0;
                glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &v);
                // Little endian, as bytes means we get ABGR 32 bit
                int rgb = static_cast<int>(
                    ((v & 0xff) << 16) | (v & 0xff00) | ((v >> 16) & 0xff));
                fmt::print("{:08x} - > {:08x}\n", v, rgb);
                return mrb::to_value(rgb, mrb);
            }

            auto [x, y, w, h] = mrb::get_args<int, int, int, int>(mrb);
            auto* ptr = new uint32_t[w * h];
            memset(ptr, 0xff, w * h * 4);

            y = display->height - (y + h);

            glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, ptr);

            auto a = mrb_ary_new_capa(mrb, w * h);
            int i = 0;
            for (y = 0; y < h; y++) {
                for (x = 0; x < w; x++) {
                    auto v = ptr[x + (h - 1 - y) * w];
                    int rgb = static_cast<int>(
                        ((v & 0xff) << 16) | (v & 0xff00) | ((v >> 16) & 0xff));
                    fmt::print("{:08x} - > {:08x}\n", v, rgb);
                    mrb_ary_set(mrb, a, i++, mrb_int_value(mrb, rgb));
                }
            }
            return a;
        },
        MRB_ARGS_REQ(4));

    mrb_define_method(
        ruby, Display::rclass, "bench_start",
        [](mrb_state* /*mrb*/, mrb_value self) -> mrb_value {
            auto* display = mrb::self_to<Display>(self);
            display->bench_start = clk::now();
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));
    mrb_define_method(
        ruby, Display::rclass, "bench_end",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* display = mrb::self_to<Display>(self);
            auto [i] = mrb::get_args<int>(mrb);
            display->bench_times[i] =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    clk::now() - display->bench_start)
                    .count();
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    mrb_define_method(
        ruby, Display::rclass, "clear",
        [](mrb_state* /*mrb*/, mrb_value self) -> mrb_value {
            auto* display = mrb::self_to<Display>(self);
            for (auto&& layer : display->layers) {
                layer->clear();
            }
            display->end_draw();
            return mrb_nil_value();
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, Display::rclass, "bg=",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto [av] = mrb::get_args<mrb_value>(mrb);
            auto* display = mrb::self_to<Display>(self);
            display->bg = mrb::to_array<float, 4>(av, mrb);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));
    mrb_define_method(
        ruby, Display::rclass, "bg",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* display = mrb::self_to<Display>(self);
            return mrb::to_value(display->bg, mrb);
        },
        MRB_ARGS_NONE());
}
