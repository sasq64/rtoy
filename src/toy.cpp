
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

#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
namespace fs = std::filesystem;

using namespace std::string_literals;
static std::string to_run;

void Toy::init()
{
    ruby = mrb_open();

    auto define_const = [&](RClass* mod, std::string const& sym, uint32_t v) {
        auto sv = static_cast<int32_t>(v);
        mrb_define_const(ruby, mod, sym.c_str(), mrb_int_value(ruby, sv));
    };

    auto* colors = mrb_define_class(ruby, "Color", ruby->object_class);
    define_const(colors, "BLACK", 0x000000ff);
    define_const(colors, "WHITE", 0xffffffff);
    define_const(colors, "RED", 0x880000ff);
    define_const(colors, "CYAN", 0xAAFFEEff);
    define_const(colors, "PURPLE", 0xcc44ccff);
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
    puts("RFont");
    RFont::reg_class(ruby);
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
    puts("RAudio");
    RAudio::reg_class(ruby);
    puts("RSpeech");
    RSpeech::reg_class(ruby);

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
        ruby, ruby->kernel_module, "require",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            auto [name] = mrb::get_args<std::string>(mrb);
            fmt::print("Require {}\n", name);
            auto p = ruby_path / name;
            if (p.extension() == "") { p.replace_extension(".rb"); }
            if (fs::exists(p)) {
                auto cp = fs::canonical(p);
                if (already_loaded.count(cp) > 0) {
                    fmt::print("{} already loaded\n", cp.string());
                    return mrb_nil_value();
                }
                already_loaded.insert(cp);

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

Toy::Toy(bool fs)
{
    Display::full_screen = fs;
}

int Toy::run(std::string const& script)
{
    init();
    puts("Main");
    std::ifstream ruby_file;
    ruby_file.open(script);
    auto source = read_all(ruby_file);
    mrb_load_string(ruby, source.c_str());
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
