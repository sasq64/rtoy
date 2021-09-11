#include "rdisplay.hpp"

#include "mruby/value.h"
#include "rcanvas.hpp"
#include "rconsole.hpp"
#include "rimage.hpp"
#include "rsprites.hpp"

#include "error.hpp"
#include "gl/functions.hpp"
#include "mrb_tools.hpp"

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

mrb_data_type Display::dt{"Display", [](mrb_state*, void* data) {
                              auto* display = static_cast<Display*>(data);
                              // SET_NIL_VALUE(display->draw_handler);
                              if (display != Display::default_display) {
                                  delete display;
                              }
                          }};

Display::Display(mrb_state* state, System& system, Settings const& _settings)
    : ruby(state), settings{_settings}, RLayer(0, 0)
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

    fmt::print("{}\n", settings.console_font.string());

    console = std::make_shared<RConsole>(w, h,
        Style{0xffffffff, 0x00008000, settings.console_font.string(),
            settings.font_size});
    puts("Console");
    gl::setViewport({w, h});
    fmt::print("{} {}\n", w, h);
    canvas = std::make_shared<RCanvas>(w, h);
    canvas->init(ruby);
    sprites = std::make_shared<RSprites>(w, h);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // GLuint vao;
    // glGenVertexArrays(1, &vao);
    // glBindVertexArray(vao);
}

bool Display::begin_draw()
{
    pix::set_transform(Id);
    call_proc(ruby, draw_handler, 0);
    return false;
}

void Display::end_draw()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gl::clearColor({bg});
    glClear(GL_COLOR_BUFFER_BIT);

    gl::setViewport({width, height});
    console->render();
    canvas->render();
    sprites->render();
}

void Display::swap()
{
    window->swap();
}

void Display::reset()
{
    bg = {0, 0, 0.8, 1.0};

    if (mouse_cursor != nullptr) {
        sprites->remove_sprite(mouse_cursor);
        mouse_cursor = nullptr;
    }
    console->reset();
    canvas->reset();
    sprites->reset();
    SET_NIL_VALUE(draw_handler);
}

void Display::reg_class(
    mrb_state* ruby, System& system, Settings const& settings)
{
    Display::rclass = mrb_define_class(ruby, "Display", RLayer::rclass);
    MRB_SET_INSTANCE_TT(Display::rclass, MRB_TT_DATA);

    if (Display::default_display == nullptr) {
        Display::default_display = new Display(ruby, system, settings);
    } else {
        Display::default_display->ruby = ruby;
    }

    Display::default_display->disp_obj =
        mrb::new_data_obj(ruby, Display::default_display);
    mrb_gc_register(ruby, Display::default_display->disp_obj);

    mrb_define_class_method(
        ruby, Display::rclass, "default",
        [](mrb_state*  /*mrb*/, mrb_value /*self*/) -> mrb_value {
            return Display::default_display->disp_obj;
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, Display::rclass, "mouse_ptr",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* display = mrb::self_to<Display>(self);
            RImage* image = nullptr;
            mrb_get_args(mrb, "d", &image, &RImage::dt);
            display->mouse_cursor = display->sprites->add_sprite(image, 1);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(3));

    mrb_define_method(
        ruby, Display::rclass, "width",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* display = mrb::self_to<Display>(self);
            return mrb::to_value(display->width, mrb);
        },
        MRB_ARGS_NONE());
    mrb_define_method(
        ruby, Display::rclass, "height",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* display = mrb::self_to<Display>(self);
            return mrb::to_value(display->height, mrb);
        },
        MRB_ARGS_NONE());
    mrb_define_method(
        ruby, Display::rclass, "on_draw",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* display = mrb::self_to<Display>(self);
            mrb_get_args(mrb, "&!", &display->draw_handler);
            mrb_gc_register(mrb, display->draw_handler);
            return mrb_nil_value();
        },
        MRB_ARGS_BLOCK());

    mrb_define_method(
        ruby, Display::rclass, "console",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* display = mrb::self_to<Display>(self);
            return mrb::new_data_obj(mrb, display->console.get());
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
        ruby, Display::rclass, "sprites",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* display = mrb::self_to<Display>(self);
            return mrb::new_data_obj(mrb, display->sprites.get());
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
        ruby, Display::rclass, "clear",
        [](mrb_state* /*mrb*/, mrb_value self) -> mrb_value {
            auto* display = mrb::self_to<Display>(self);
            display->console->clear();
            display->canvas->clear();
            display->sprites->clear();
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
