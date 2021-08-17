
#include "toy.hpp"

#include <CLI/CLI.hpp>
#include <fmt/format.h>
#include <coreutils/split.h>
#include <string>

using namespace std::string_literals;

int start_toy(std::string const& script);

int main(int argc, char const** argv)
{
    std::string main_script = "ruby/main.rb";
    bool full_screen = false;
    std::string font_name;
    std::string boot_cmd;
    CLI::App app{"toy"};
    app.set_help_flag();
    auto* help = app.add_flag("-h,--help", "Request help");
    app.add_flag("-f,--full-screen", full_screen, "Fullscreen");
    app.add_option("--main", main_script, "Main script");
    app.add_option("--boot", boot_cmd, "Command to run at bootup");
    app.add_option("--font", font_name, "Console font (ttf_file[:size])");
    app.parse(argc, argv);

    Settings settings;
    if (!font_name.empty()) {
        auto [font, size] = utils::split2(font_name, ":"s);
        fmt::print("'{}' '{}'\n", font, size);
        settings.console_font = font;
        if(!size.empty()) {
            settings.font_size = std::stoi(size);
        }
    }
    settings.boot_cmd = boot_cmd;
    settings.screen = full_screen ? ScreenType::Full : ScreenType::Window;
    settings.boot_script = main_script;
    Toy toy(settings);
    toy.run();
}
