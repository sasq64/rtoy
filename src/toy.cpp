#include "toy.hpp"
#include "error.hpp"

#include "rcanvas.hpp"
#include "rconsole.hpp"
#include "rdisplay.hpp"
#include "rimage.hpp"
#include "rinput.hpp"
#include "rsprites.hpp"
#include "rtimer.hpp"

#include "mrb_tools.hpp"

#include <pix/gl_console.hpp>

#include <mruby.h>
#include <mruby/compile.h>
#include <mruby/value.h>

#include <string>
#include <thread>

using namespace std::string_literals;
static std::string to_run;

void Toy::init()
{
    ruby = mrb_open();

    auto define_const = [&](RClass* mod, std::string const& sym, uint32_t v) {
        fmt::print("{} {:x} {}\n", sym, v, v);
        mrb_define_const(ruby, mod, sym.c_str(), mrb_int_value(ruby, v));
    };

    auto* colors = mrb_define_module(ruby, "Color");
    define_const(colors, "BLACK", 0x000000ff);
    define_const(colors, "WHITE", 0xffffffff);
    define_const(colors, "RED", 0x880000ff);
    define_const(colors, "CYAN", 0xAAFFEEff);
    define_const(colors, "PURLE", 0xcc44ccff);
    define_const(colors, "GREEN", 0x00cc55ff);
    define_const(colors, "BLUE", 0x0000aaff);
    define_const(colors, "YELLOW", 0xeeee77ff);
    define_const(colors, "ORANGE", 0xdd8855ff);
    define_const(colors, "BROWN", 0x664400ff);
    define_const(colors, "LIGHT_RED", 0xff7777ff);
    define_const(colors, "DARK_GREY", 0x333333ff);
    define_const(colors, "GREY", 0x777777ff);
    define_const(colors, "LIGHT_GREEN", 0xaaff66ff);
    define_const(colors, "LIGHT_BLUE", 0x0088ffff);
    define_const(colors, "LIGHT_GREY", 0xbbbbbbff);

    puts("RLayer");
    RLayer::reg_class(ruby);
    puts("RConsole");
    RConsole::reg_class(ruby);
    puts("RCanvas");
    RCanvas::reg_class(ruby);
    puts("RImage");
    RImage::reg_class(ruby);
    puts("RDisplay");
    Display::reg_class(ruby);
    puts("RInput");
    RInput::reg_class(ruby);
    puts("RSprites");
    RSprites::reg_class(ruby);
    puts("RTimer");
    RTimer::reg_class(ruby);

    mrb_define_module_function(
        ruby, ruby->kernel_module, "exec",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            auto [source] = mrb::get_args<std::string>(mrb);
            fmt::print("Load string {}\n", strlen(source));
            to_run = source;
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    mrb_define_module_function(
        ruby, ruby->kernel_module, "load",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            auto [name] = mrb::get_args<std::string>(mrb);
            fmt::print("Loading {}\n", name);
            FILE* fp = fopen(name, "rbe");
            if (fp != nullptr) {
                mrb_load_file(mrb, fp);
                fclose(fp);
                if (auto err = mrb::check_exception(mrb)) {
                    fmt::print("Error: {}\n", *err);
                    exit(1);
                }
            }
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    mrb_define_module_function(
        ruby, ruby->kernel_module, "require",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            auto [name] = mrb::get_args<std::string>(mrb);
            fmt::print("Loading {}\n", name);
            FILE* fp = fopen(("ruby/"s + name).c_str(), "rbe");
            if (fp != nullptr) {
                mrb_load_file(mrb, fp);
                fclose(fp);
                if (auto err = mrb::check_exception(mrb)) {
                    fmt::print("Error: {}\n", *err);
                    exit(1);
                }
            }
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    puts("Main");
    std::ifstream ruby_file;
    ruby_file.open("ruby/main.rb");
    auto source = read_all(ruby_file);
    auto v = mrb_load_string(ruby, source.c_str());
    if (auto err = mrb::check_exception(ruby)) {
        fmt::print("Error: {}\n", *err);
        exit(1);
    }
    puts("Loaded");
}

void Toy::destroy()
{
    mrb_close(ruby);
    ruby = nullptr;
}

bool Toy::render_loop()
{
    auto* display = Display::default_display;
    // auto seconds = get_seconds();
    auto* input = RInput::default_input;
    if (display->begin_draw()) { return true; }
    if ((input != nullptr) && input->update()) { return true; }

    if (!ErrorState::stack.empty()) {
        auto e = ErrorState::stack.back();
        ErrorState::stack.pop_back();
        display->reset();
        if (RInput::default_input != nullptr) {
            RInput::default_input->reset();
        }
        if (RTimer::default_timer != nullptr) {
            RTimer::default_timer->reset();
        }
        display->console->clear();
        display->console->console->text(0, 0, e.text);
    }

    if ((input != nullptr) && input->should_reset()) {
        display->setup();
        display->reset();
        input->reset();
        RTimer::default_timer->reset();
        std::ifstream ruby_file;
        ruby_file.open("ruby/main.rb");
        auto source = read_all(ruby_file);
        auto v = mrb_load_string(ruby, source.c_str());
    }

    if (RTimer::default_timer != nullptr) { RTimer::default_timer->update(); }

    display->end_draw();

    if (!to_run.empty()) {
        fmt::print("Running\n");
        mrb_load_string(ruby, to_run.c_str());
        if (auto err = mrb::check_exception(ruby)) {
            ErrorState::stack.push_back(
                {ErrorType::Exception, std::string(*err)});
            fmt::print("Error: {}\n", *err);
        }
        to_run.clear();
    }

    return false;
}

#ifdef __EMSCRIPTEN__
#    include <emscripten.h>
#endif

int Toy::run(std::string const& script)
{
    init();
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(
        [](void* data) {
            auto* toy = static_cast<Toy*>(data);
            auto quit = toy->render_loop();
        },
        this, 0, true);
#else
    bool quit = false;
    while (!quit) {
        quit = render_loop();
    }
    destroy();
#endif
    return 0;
}
