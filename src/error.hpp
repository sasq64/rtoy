#pragma once
#include "mrb_tools.hpp"
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
    std::string text;
};

class ErrorState
{
public:
    static inline std::deque<Error> stack;
};

template <typename... T>
bool call_proc(mrb_state* ruby, mrb_value handler, T... arg)
{
    if (handler.w == 0) { return false; }
    mrb_funcall(
        ruby, handler, "call", sizeof...(arg), mrb::to_value(arg, ruby)...);
    if (ruby->exc != nullptr) {
        auto obj = mrb_funcall(ruby, mrb_obj_value(ruby->exc), "inspect", 0);
        std::string err(RSTRING_PTR(obj), RSTRING_LEN(obj));

        ErrorState::stack.push_back({ErrorType::Exception, err});
        fmt::print("Error: {}\n", err);
        ruby->exc = nullptr;
        return true;
    }
    return false;
}
