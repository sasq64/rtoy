
#include <memory>
#include <mruby.h>
#include <mruby/data.h>
#include <mruby/value.h>
#include <string>

#include <fstream>
#include <filesystem>
#include <set>

class Toy
{
    template <typename Stream>
    static std::string read_all(Stream& in)
    {
        std::string contents;
        auto here = in.tellg();
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg() - here);
        in.seekg(here, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
        return contents;
    }

    mrb_state* ruby;
    static void exec(mrb_state* mrb, std::string const& code);

    static inline std::filesystem::path ruby_path = "ruby";
    static inline std::set<std::filesystem::path> already_loaded{};
public:


    void init();
    void destroy();

    bool render_loop();
    int run(std::string const& script);
};
