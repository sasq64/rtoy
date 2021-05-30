#include "toy.hpp"

#include <mruby.h>
#include <mruby/compile.h>
#include <mruby/value.h>

#include <filesystem>
#include <pix/gl_console.hpp>
#include <string>
#include <thread>

#include "error.hpp"
#include "mrb_tools.hpp"
#include "rcanvas.hpp"
#include "rconsole.hpp"
#include "rdisplay.hpp"
#include "rimage.hpp"
#include "rinput.hpp"
#include "rsprites.hpp"
#include "rtimer.hpp"
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

    auto* colors = mrb_define_module(ruby, "Color");
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
        ruby, ruby->kernel_module, "puts",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            auto [s] = mrb::get_args<std::string>(mrb);
            auto text = std::string(s) + "\n";
            Display::default_display->console->text(text);
            return mrb_nil_value();
        },
        MRB_ARGS_REQ(1));

    mrb_define_module_function(
        ruby, ruby->kernel_module, "print",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            auto [text] = mrb::get_args<std::string>(mrb);
            Display::default_display->console->text(text);
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
                //mrb_load_file(mrb, fp);
                fseek(fp, 0, SEEK_END);
                auto sz = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                std::string s;
                s.resize(sz + 1);
                fread(s.data(), sz, 1, fp);
                s[sz] = 0;
                fclose(fp);
                exec(mrb, s);
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
                    mrb_load_file(mrb, fp);
                    fclose(fp);
                    if (auto err = mrb::check_exception(mrb)) {
                        fmt::print("Error: {}\n", *err);
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
            for (auto&& p : fs::directory_iterator(parent)) {
                auto real_path = dir == "." ? p.path().filename()
                                            : dir / p.path().filename();
                fmt::print("{}/{}\n", p.path().string(), real_path.string());
                files.emplace_back(real_path);
            }
            return mrb::to_value(files, mrb);
        },
        MRB_ARGS_REQ(1));
}

void Toy::exec(mrb_state* mrb, std::string const& code)
{
    // auto* cxt = mrbc_context_new(mrb);
    // cxt->capture_errors = TRUE;
    // cxt->lineno = 1;
    auto ai = mrb_gc_arena_save(mrb);
    auto* parser = mrb_parser_new(mrb);
    if (parser == NULL) {
        fputs("create parser state error\n", stderr);
        return;
    }
    parser->s = code.c_str();
    parser->send = code.c_str() + code.length();
    parser->lineno = 1;
    mrbc_context* cxt = nullptr;
    mrb_parser_parse(parser, cxt);
    if (parser->nwarn > 0) {
        char* msg = mrb_locale_from_utf8(parser->warn_buffer[0].message, -1);
        printf("line %d: %s\n", parser->warn_buffer[0].lineno, msg);
        mrb_locale_free(msg);
        return;
    } else if (parser->nerr > 0) {
        char* msg = mrb_locale_from_utf8(parser->error_buffer[0].message, -1);
        printf("line %d: %s\n", parser->error_buffer[0].lineno, msg);
        mrb_locale_free(msg);
        return;
    }
    struct RProc* proc = mrb_generate_code(mrb, parser);
    mrb_parser_free(parser);
    if (proc == NULL) { return; }

    if (mrb->c->cibase->u.env) {
        struct REnv* e = mrb_vm_ci_env(mrb->c->cibase);
        if (e && MRB_ENV_LEN(e) < proc->body.irep->nlocals) {
            printf("MODIFY\n");
            MRB_ENV_SET_LEN(e, proc->body.irep->nlocals);
        }
    }

    struct RClass* target = mrb->object_class;
    MRB_PROC_SET_TARGET_CLASS(proc, target);
    if (mrb->c->ci) { mrb_vm_ci_target_class_set(mrb->c->ci, target); }

    unsigned stack_keep = 0;

    auto result = mrb_top_run(mrb, proc, mrb_top_self(mrb), stack_keep);
    /* auto* e = mrb_vm_top_start(mrb, proc, mrb_top_self(mrb), stack_keep); */
    /* auto* val = mrb_vm_run_cycles(mrb, e, 10); */
    /* if(val == nullptr) { */
    /*     val = mrb_vm_run_cycles(mrb, e, 0x7fffffff); */
    /* } */
    /* mrb_vm_end(mrb, e); */
    if (mrb->exc) {
        fmt::print("EXCEPTION!\n");
    } else {
        *(mrb->c->ci->stack + 1) = result;
    }

    mrb_gc_arena_restore(mrb, ai);
    //mrb_parser_free(parser);
    // mrbc_context_free(mrb, cxt);
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
    puts("Main");
    std::ifstream ruby_file;
    ruby_file.open(script);
    auto source = read_all(ruby_file);
    mrb_load_string(ruby, source.c_str());
    if (auto err = mrb::check_exception(ruby)) {
        fmt::print("Error: {}\n", *err);
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
