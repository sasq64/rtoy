
#include "toy.hpp"

#include <mruby.h>
#include <mruby/compile.h>
#include <mruby/value.h>

#include <pix/gl_console.hpp>

#include "error.hpp"
#include "mrb_tools.hpp"
#include "raudio.hpp"
#include "rcanvas.hpp"
#include "rconsole.hpp"
#include "rdisplay.hpp"
#include "rfont.hpp"
#include "rimage.hpp"
#include "rinput.hpp"
#include "rspeech.hpp"
#include "rsprites.hpp"
#include "rtimer.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

namespace fs = std::filesystem;
using clk = std::chrono::steady_clock;
using namespace std::string_literals;
static std::string to_run;

template <typename TP>
std::time_t to_time_t(TP tp)
{
    using namespace std::chrono; // NOLINT
    auto sctp = time_point_cast<system_clock::duration>(
        tp - TP::clock::now() + system_clock::now());
    return system_clock::to_time_t(sctp);
}

void Toy::init()
{
    ruby = mrb_open();

#ifdef RASPBERRY_PI
    system = create_pi_system();
#else
    system = create_sdl_system();
#endif

    RLayer::reg_class(ruby);
    RConsole::reg_class(ruby);
    RCanvas::reg_class(ruby);
    RFont::reg_class(ruby);
    RImage::reg_class(ruby);
    Display::reg_class(ruby, *system, settings);
    RInput::reg_class(ruby, *system);
    RSprites::reg_class(ruby);
    RTimer::reg_class(ruby);
    RAudio::reg_class(ruby, *system, settings);
    RSpeech::reg_class(ruby);

    auto* rclass = mrb_define_class(ruby, "Settings", nullptr);
    mrb_define_const(
        ruby, rclass, "BOOT_CMD", mrb::to_value(settings.boot_cmd, ruby));
    mrb_define_const(ruby, rclass, "CONSOLE_FONT",
        mrb::to_value(settings.console_font.string(), ruby));

    mrb_define_module_function(
        ruby, ruby->kernel_module, "puts",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            auto n = mrb_get_argc(mrb);
            if (n == 0) {
                Display::default_display->console->text("\n");
                return mrb_nil_value();
            }
            auto [val] = mrb::get_args<mrb_value>(mrb);
            // TODO: Dont call if already string
            auto sval = mrb_funcall(mrb, val, "to_s", 0);

            auto text = mrb::to<std::string>(sval) + "\n";
            Display::default_display->console->text(text);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    mrb_define_module_function(
        ruby, ruby->kernel_module, "assert",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            auto [what] = mrb::get_args<bool>(mrb);
            assert(what);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    mrb_define_module_function(
        ruby, ruby->kernel_module, "file_time",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            auto [name] = mrb::get_args<std::string>(mrb);
            auto rb_file = fs::canonical(name);
            auto modified = fs::last_write_time(rb_file);
            auto tt = static_cast<double>(to_time_t(modified));
            return mrb::to_value(tt, mrb);
        },
        MRB_ARGS_REQ(1));

    mrb_define_module_function(
        ruby, ruby->kernel_module, "require",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            auto [name] = mrb::get_args<std::string>(mrb);
            fmt::print("Require {}\n", name);
            auto rb_file = ruby_path / name;
            if (rb_file.extension() == "") { rb_file.replace_extension(".rb"); }
            if (fs::exists(rb_file)) {
                rb_file = fs::canonical(rb_file);
                auto modified = fs::last_write_time(rb_file);

                auto it = already_loaded.find(rb_file.string());

                if (it != already_loaded.end() && it->second == modified) {
                    fmt::print("{} already loaded\n", rb_file.string());
                    return mrb_nil_value();
                }
                already_loaded[rb_file.string()] = modified;

                FILE* fp = fopen(("ruby/"s + name).c_str(), "rbe");
                if (fp != nullptr) {
                    auto* ctx = mrbc_context_new(mrb);
                    ctx->capture_errors = true;
                    mrbc_filename(mrb, ctx, name.c_str());
                    ctx->lineno = 1;
                    mrb_load_file_cxt(mrb, fp, ctx);
                    mrbc_context_free(mrb, ctx);
                    fclose(fp);
                    if (auto err = mrb::check_exception(mrb)) {
                        fmt::print("REQUIRE Error: {}\n", *err);
                        exit(1);
                    }
                }
            } else {
                mrb_raise(mrb, mrb->object_class,
                    fmt::format("'{}' not found", name).c_str());
            }
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    static auto root_path = fs::current_path();

    mrb_define_module_function(
        ruby, ruby->kernel_module, "list_files",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            auto [ds] = mrb::get_args<std::string>(mrb);
            auto dir = fs::path(ds);
            auto parent = fs::canonical(root_path / dir);
            std::vector<std::string> files;
            if (fs::is_directory(parent)) {
                for (auto&& p : fs::directory_iterator(parent)) {
                    auto real_path = dir == "." ? p.path().filename()
                                                : dir / p.path().filename();
                    fmt::print("{}\n", real_path.string());
                    files.emplace_back(real_path);
                }
            }
            return mrb::to_value(files, mrb);
        },
        MRB_ARGS_REQ(1));
}

void Toy::exec(mrb_state* mrb, std::string const& code)
{
    auto* ctx = mrbc_context_new(mrb);
    ctx->capture_errors = TRUE;
    ctx->lineno = 1;

    // auto ai = mrb_gc_arena_save(ruby);

    auto* parser = mrb_parser_new(mrb);
    if (parser == nullptr) { throw toy_exception("Can't create parser"); }

    parser->s = code.c_str();
    parser->send = code.c_str() + code.length();
    parser->lineno = 1;
    mrb_parser_parse(parser, ctx);

    if (parser->nwarn > 0) {
        char* msg = mrb_locale_from_utf8(parser->warn_buffer[0].message, -1);
        printf("line %d: %s\n", parser->warn_buffer[0].lineno, msg);
        mrb_locale_free(msg);
        return;
    }
    if (parser->nerr > 0) {
        char* msg = mrb_locale_from_utf8(parser->error_buffer[0].message, -1);
        printf("line %d: %s\n", parser->error_buffer[0].lineno, msg);
        mrb_locale_free(msg);
        return;
    }
    struct RProc* proc = mrb_generate_code(mrb, parser);
    mrb_parser_free(parser);
    if (proc == nullptr) { throw toy_exception("Can't generate code"); }
    // struct RClass* target = mrb->object_class;
    // MRB_PROC_SET_TARGET_CLASS(proc, target);
    auto result = mrb_vm_run(mrb, proc, mrb_top_self(mrb), stack_keep);
    // stack_keep = proc->body.irep->nlocals;
    // mrb_gc_arena_restore(ruby, ai);
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
        int y = 2;
        for (auto& bt : e.backtrace) {
            display->console->console->text(2, y++, bt);
        }
    }

    if ((input != nullptr) && input->should_reset()) {
        display->reset();
        input->reset();
        RTimer::default_timer->reset();
        std::ifstream ruby_file;
        ruby_file.open("ruby/main.rb");
        auto source = read_all(ruby_file);
        mrb_load_string(ruby, source.c_str());
    }

    if (RTimer::default_timer != nullptr) { RTimer::default_timer->update(); }

    display->end_draw();

    if (!to_run.empty()) {
        fmt::print("Running\n");
        mrb_load_string(ruby, to_run.c_str());
        if (auto err = mrb::check_exception(ruby)) {
            ErrorState::stack.push_back(
                {ErrorType::Exception, {}, std::string(*err)});
            fmt::print("RUN Error: {}\n", *err);
        }
        to_run.clear();
    }

    return false;
}

#ifdef __EMSCRIPTEN__
#    include <emscripten.h>
#endif

Toy::Toy(Settings const& _settings) : settings{_settings}
{
    Display::full_screen = settings.screen == ScreenType::Full;
}

int Toy::run()
{
    init();

    auto con = Display::default_display->console->console;

    if(settings.console_benchmark) {
        for (int i = 0; i < 500; i++) {
            con->fill(0xff00ff00, 0x00ff00ff);
            con->flush();
            con->fill(0x00ff00ff, 0xff00ff00);
            con->flush();
        }
        return 0;
    }

    puts("Main");
    std::ifstream ruby_file;
    ruby_file.open(settings.boot_script);
    auto source = read_all(ruby_file);
    exec(ruby, source);
    if (auto err = mrb::check_exception(ruby)) {
        fmt::print("START Error: {}\n", *err);
        for (auto&& line : mrb::get_backtrace(ruby)) {
            fmt::print("  {}", line);
        }
        exit(1);
    }
    puts("Loaded");
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
