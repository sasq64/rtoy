
#include "toy.hpp"

#include <CLI/CLI.hpp>
#include <string>

int start_toy(std::string const& script);

int main(int argc, char const** argv)
{
    std::string main_script = "ruby/main.rb";
    CLI::App app{"toy"};
    app.set_help_flag();
    auto* help = app.add_flag("-h,--help", "Request help");
    app.add_option("--main", main_script, "Main script");
    app.parse(argc, argv);

    Toy toy{};
    toy.run(main_script);

}
