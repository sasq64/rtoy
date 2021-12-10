
#include "toy.hpp"

#include "error.hpp"
#include "mrb/class.hpp"
#include "mrb/mrb_tools.hpp"
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

#include <pix/pixel_console.hpp>

#include <coreutils/split.h>

#include <mruby.h>
#include <mruby/compile.h>
#include <mruby/value.h>

#ifdef __EMSCRIPTEN__
#    include <emscripten.h>
#endif

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

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

extern "C" void send_to_rtoy(const char* text)
{
    auto* inp = RInput::default_input;
    for (auto c : std::string(text)) {
        inp->put_char(c);
    }
}

fs::path find_data_root()
{
    fs::path d = fs::current_path();
    while (true) {
        if (fs::exists(d / "data") && fs::exists(d / "sys")) { return d; }
        d = d.parent_path();
        if (d.empty()) { break; }
    }
    return {};
}

void Toy::require(mrb_state* mrb, std::string const& name)
{
    fmt::print("Require {}\n", name);

    auto parts = utils::split(ruby_path, ":"s);
    std::optional<fs::path> to_load;
    for (auto const& part : parts) {
        auto rb_file = fs::path(part) / name;
        if (rb_file.extension() == "") { rb_file.replace_extension(".rb"); }
        if (fs::exists(rb_file)) {
            to_load = rb_file;
            break;
        }
    }
    if (!to_load) {
        mrb_raise(mrb, mrb->object_class,
            fmt::format("'{}' not found", name).c_str());
    }
    auto rb_file = fs::canonical(*to_load);
    auto modified = fs::last_write_time(rb_file);

    auto it = already_loaded.find(rb_file.string());

    if (it != already_loaded.end() && it->second == modified) {
        fmt::print("{} already loaded\n", rb_file.string());
        return;
    }
    already_loaded[rb_file.string()] = modified;

    FILE* fp = fopen(rb_file.string().c_str(), "rb");
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
            ::exit(1);
        }
    }
}

void Toy::init()
{
    ruby = mrb_open();

#ifdef RASPBERRY_PI
    system = create_pi_system();
#else
    system = create_sdl_system();
#endif

    data_root = find_data_root();
    if (data_root.empty()) {
        fprintf(stderr, "**Error: Can not find data files!\n");
        ::exit(1);
    }
    fs::current_path(data_root);
    fs::copy(
        "sys/help.rb", "ruby/help.rb", fs::copy_options::overwrite_existing);

    RLayer::reg_class(ruby);
    RConsole::reg_class(ruby);
    RCanvas::reg_class(ruby);
    RFont::reg_class(ruby);
    RSprites::reg_class(ruby);

    RImage::reg_class(ruby);
    RInput::reg_class(ruby, *system);
    RTimer::reg_class(ruby);

    RAudio::reg_class(ruby, *system, settings);
    RSpeech::reg_class(ruby);

    Display::reg_class(ruby, *system, settings);

    fmt::print("SYSTEM: {}\n", settings.system);
    auto* rclass = mrb::make_noinit_class<Settings>(ruby, "Settings");
    //mrb::define_const<Settings>(ruby, "SYSTEM", mrb::Symbol{ruby, settings.system});

    //mrb_intern_cstr(ruby, settings.system.c_str());
    mrb_define_const(ruby, rclass, "SYSTEM",
        mrb_check_intern_cstr(ruby, settings.system.c_str()));

    mrb::define_const<Settings>(ruby, "BOOT_CMD", settings.boot_cmd);
    mrb::define_const<Settings>(
        ruby, "CONSOLE_FONT", settings.console_font.string());

    mrb::add_kernel_function(ruby, "puts", [](mrb_state* mrb, mrb_value val) {
        auto sval = mrb_funcall(mrb, val, "to_s", 0);
        auto text = mrb::value_to<std::string>(sval) + "\n";
        Display::default_display->console->text(text);
    });

    mrb::add_kernel_function(ruby, "exit", [] {
        Toy::exit();
        puts("EXIT");
    });
    mrb::add_kernel_function(ruby, "assert", [](bool what) { assert(what); });

    mrb::add_kernel_function(ruby, "file_time", [](std::string const& name) {
        auto rb_file = fs::canonical(name);
        auto modified = fs::last_write_time(rb_file);
        return static_cast<double>(to_time_t(modified));
    });

    mrb::add_kernel_function(
        ruby, "require", [](mrb_state* mrb, std::string const& name) {
            Toy::require(mrb, name);
        });

    static auto root_path = fs::current_path();

    mrb::add_kernel_function(
        ruby, "list_files", [](std::string const& name) {
            auto dir = fs::path(name);
            auto parent = fs::canonical(root_path / dir);
            std::vector<std::string> files;
            if (fs::is_directory(parent)) {
                for (auto&& p : fs::directory_iterator(parent)) {
                    auto real_path = p.path().filename(); //
                    fmt::print("{}\n", real_path.string());
                    files.emplace_back(real_path.string());
                }
            }
            return files;
        });
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
    /* auto result = */ mrb_vm_run(mrb, proc, mrb_top_self(mrb), stack_keep);
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
    auto t = clk::now();
    RAudio::default_audio->update();

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
        display->console->text(0, 0, e.text);
        int y = 2;
        for (auto& bt : e.backtrace) {
            display->console->text(2, y++, bt);
        }
    }

    if ((input != nullptr) && input->should_reset()) {
        display->reset();
        input->reset();
        RTimer::default_timer->reset();
        std::ifstream ruby_file;
        ruby_file.open("sys/main.rb");
        auto source = read_all(ruby_file);
        mrb_load_string(ruby, source.c_str());
    }

    auto [mx, my] = input->mouse_pos();
    if (display->mouse_cursor != nullptr) {
        display->mouse_cursor->trans = {
            static_cast<float>(mx), static_cast<float>(my)};
        display->mouse_cursor->dirty = true;
    }

    if (RTimer::default_timer != nullptr) { RTimer::default_timer->update(); }

    auto rt = clk::now() - t;
    display->pre_t =
        std::chrono::duration_cast<std::chrono::milliseconds>(rt).count();

    display->end_draw();
    display->swap();

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

Toy::Toy(Settings const& _settings) : settings{_settings} {}

int Toy::run()
{
    init();

    if (settings.console_benchmark) {
        auto con = Display::default_display->console->console;
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
        return -1;
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
    while (!quit && !do_exit) {
        quit = render_loop();
    }
    destroy();
#endif
    return 0;
}
