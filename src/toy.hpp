#pragma once

#include "settings.hpp"
#include "system.hpp"

#include <mruby.h>
#include <mruby/data.h>
#include <mruby/value.h>

#include <ctime>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

struct toy_exception : public std::exception
{
    explicit toy_exception(std::string const& text) : msg(text) {}
    std::string msg;
    char const* what() const noexcept override { return msg.c_str(); }
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

    Settings const& settings;
    std::unique_ptr<System> system;

    std::filesystem::path data_root;

    mrb_state* ruby = nullptr;
    static inline int stack_keep = 0;

    // namespace fs = std::filesystem;
    using path = std::filesystem::path;

    static inline std::string ruby_path = "ruby:sys";
    static inline std::unordered_map<std::string,
        std::filesystem::file_time_type>
        already_loaded;

public:
    explicit Toy(Settings const& settings);

    static void exec(mrb_state* mrb, std::string const& code);
    void init();
    void destroy();

    bool render_loop();
    int run();
};
