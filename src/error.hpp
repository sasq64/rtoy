#pragma once
#include "mrb/mrb_tools.hpp"
#include "mruby/array.h"
#include <deque>
#include <mruby/boxing_word.h>
#include <string>

enum class ErrorType
{
    None,
    Exception,
    Reset,
};

struct Error
{
    ErrorType type;
    std::vector<std::string> backtrace;
    std::string text;
};

class ErrorState
{
public:
    static inline mrb_value error_handler{};
    static inline std::deque<Error> stack;
};

template <typename... T>
bool call_proc(mrb_state* ruby, mrb_value handler, T... arg)
{
    if (handler.w == 0) { return false; }
    mrb_funcall(
        ruby, handler, "call", sizeof...(arg), mrb::to_value(arg, ruby)...);
    if (ruby->exc != nullptr) {

        if (ErrorState::error_handler.w != 0) {
            auto rc = call_proc(
                ruby, ErrorState::error_handler, mrb_obj_value(ruby->exc));
            return rc;
        }

        auto bt = mrb_funcall(ruby, mrb_obj_value(ruby->exc), "backtrace", 0);

        std::vector<std::string> backtrace;
        for (int i = 0; i < ARY_LEN(mrb_ary_ptr(bt)); i++) {
            auto v = mrb_ary_entry(bt, i);
            auto s = mrb_funcall(ruby, v, "to_s", 0);
            std::string line(RSTRING_PTR(s), RSTRING_LEN(s));
            fmt::print("LINE:{}\n", line);
            backtrace.emplace_back(line);
        }

        auto obj = mrb_funcall(ruby, mrb_obj_value(ruby->exc), "inspect", 0);
        std::string err(RSTRING_PTR(obj), RSTRING_LEN(obj));

        ErrorState::stack.push_back({ErrorType::Exception, backtrace, err});
        fmt::print("Error: {}\n", err);
        ruby->exc = nullptr;
        return true;
    }
    return false;
}
