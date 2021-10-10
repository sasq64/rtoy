
#include "toy.hpp"

#include <CLI/CLI.hpp>
#include <coreutils/split.h>
#include <fmt/format.h>
#include <string>

using namespace std::string_literals;

int main(int argc, char const** argv)
{
    std::string main_script = "sys/main.rb";
    bool full_screen = false;
    bool hidden = false;
    std::string font_name;
    std::string boot_cmd;
    Settings settings;

#ifdef RASPBERRY_PI
    settings.system = "raspberry_pi";
#elif (defined __EMSCRIPTEN__)
    settings.system = "emscripten";
#else
    settings.system = "posix";
#endif

    CLI::App app{"toy"};
    app.set_help_flag();
    auto* help = app.add_flag("-h,--help", "Request help");
    app.add_flag("-f,--full-screen", full_screen, "Fullscreen");
    app.add_flag("-t,--hidden", hidden, "Hide window");
    app.add_option("--boot", settings.boot_cmd, "Command to run at bootup");
    app.add_option("--main", settings.boot_script, "Main script");
    app.add_option("--font", font_name, "Console font (ttf_file[:size])");
    app.add_option("--console-benchmark", settings.console_benchmark,
        "Check speed of console");
    CLI11_PARSE(app, argc, argv);

    if (!font_name.empty()) {
        auto [font, size] = utils::split2(font_name, ":"s);
        fmt::print("'{}' '{}'\n", font, size);
        settings.console_font = font;
        if (!size.empty()) { settings.font_size = std::stoi(size); }
    }
    settings.screen = full_screen ? ScreenType::Full : ScreenType::Window;
    if (hidden) { settings.screen = ScreenType::None; }

    Toy toy(settings);
    toy.run();
}
