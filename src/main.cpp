
#include "toy.hpp"

#include <CLI/CLI.hpp>
#include <string>

int start_toy(std::string const& script);

int main(int argc, char const** argv)
{
    std::string main_script = "ruby/main.rb";
    bool full_screen = false;
    CLI::App app{"toy"};
    app.set_help_flag();
    auto* help = app.add_flag("-h,--help", "Request help");
    app.add_flag("-f,--full-screen", full_screen, "Fullscreen");
    app.add_option("--main", main_script, "Main script");
    app.parse(argc, argv);

    ToySettings settings;
    settings.screen = full_screen ? ScreenType::Full : ScreenType::Window;
    settings.boot_script = main_script;
    Toy toy(settings);
    toy.run();
}
