
#include <ctime>
#include <memory>
#include <mruby.h>
#include <mruby/data.h>
#include <mruby/value.h>
#include <string>

#include <filesystem>
#include <unordered_map>

struct toy_exception : public std::exception
{
    explicit toy_exception(std::string const& text) : msg(text) {}
    std::string msg;
    char const* what() const noexcept override { return msg.c_str(); }
};

enum ScreenType
{
    Full,
    Window,
    None
};

struct ToySettings
{
    using path = std::filesystem::path;
    ScreenType screen = ScreenType::Window;
    path boot_script = "main.rb";
    path console_font = "";
};

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

    mrb_state* ruby = nullptr;
    static inline int stack_keep = 0;

    //namespace fs = std::filesystem;
    using path = std::filesystem::path;

    path main_script;
    static inline path ruby_path = "ruby";
    static inline std::unordered_map<std::string,
        std::filesystem::file_time_type>
        already_loaded;

public:
    explicit Toy(ToySettings const& settings);

    static void exec(mrb_state* mrb, std::string const& code);
    void init();
    void destroy();

    bool render_loop();
    int run();
};
