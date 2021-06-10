#include "rdisplay.hpp"

#include "rcanvas.hpp"
#include "rconsole.hpp"
#include "rimage.hpp"
#include "rsprites.hpp"

#include "error.hpp"
#include "gl/functions.hpp"
#include "mrb_tools.hpp"
#include "pix/gl_console.hpp"
#include <SDL.h>
#include <SDL_video.h>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <mruby/compile.h>
#include <pix/pix.hpp>
#ifdef __APPLE__
    #include "TargetConditionals.h"
    #if TARGET_OS_OSX
        #define OSX
    #endif
#endif

mrb_data_type Display::dt{"Display", [](mrb_state*, void* data) {
                              auto* display = static_cast<Display*>(data);
                              // SET_NIL_VALUE(display->draw_handler);
                              if (display != Display::default_display) {
                                  delete display;
                              }
                          }};

Display::Display(mrb_state* state) : ruby(state), RLayer(0, 0)
{
    glm::mat4x4 m(1.0F);
    memcpy(Id.data(), glm::value_ptr(m), 16 * 4);

    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Toy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
#ifdef OSX
        w/2, h/2,
        SDL_WINDOW_ALLOW_HIGHDPI |
#else
        w, h,
#endif
        SDL_WINDOW_OPENGL | (full_screen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
    // SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    // SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_CreateContext(window);
    GLenum err = glewInit();
    setup();
}

void Display::setup()
{
    SDL_GL_GetDrawableSize(window, &w, &h);
    RLayer::width = w;
    RLayer::height = h;

    console = std::make_shared<RConsole>(
        w, h, Style{0x8888ffff, 0x00008000});
    puts("Console");
    gl::setViewport({w, h});
    fmt::print("{} {}\n", w, h);
    canvas = std::make_shared<RCanvas>(w, h);
    sprites = std::make_shared<RSprites>(w, h);
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
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gl::clearColor({bg});
    glClear(GL_COLOR_BUFFER_BIT);

    gl::setViewport({w, h});
    console->render();
    canvas->render();
    sprites->render();

    SDL_GL_SwapWindow(window);
}

void Display::reset()
{
    console->clear();
    canvas->clear();
    console->reset();
    canvas->reset();
    sprites->reset();
    SET_NIL_VALUE(draw_handler);
}

void Display::reg_class(mrb_state* ruby)
{
    Display::rclass = mrb_define_class(ruby, "Display", RLayer::rclass);
    MRB_SET_INSTANCE_TT(Display::rclass, MRB_TT_DATA);

    if (Display::default_display == nullptr) {
        Display::default_display = new Display(ruby);
    } else {
        Display::default_display->ruby = ruby;
    }

    Display::default_display->disp_obj =
        mrb::new_data_obj(ruby, Display::default_display);
    mrb_gc_register(ruby, Display::default_display->disp_obj);

    mrb_define_class_method(
        ruby, Display::rclass, "default",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            return Display::default_display->disp_obj;
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
            auto* console = display->console.get();
            return mrb::new_data_obj(mrb, console);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, Display::rclass, "canvas",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* display = mrb::self_to<Display>(self);
            auto* canvas = display->canvas.get();
            return mrb::new_data_obj(mrb, canvas);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, Display::rclass, "sprites",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* display = mrb::self_to<Display>(self);
            auto* sprites = display->sprites.get();
            return mrb::new_data_obj(mrb, sprites);
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
            auto [bg] = mrb::get_args<uint32_t>(mrb);
            auto* display = mrb::self_to<Display>(self);
            display->bg = bg;
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
