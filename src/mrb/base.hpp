#pragma once
#include <stdexcept>
#include <string>

namespace mrb {

struct mrb_exception : public std::exception
{
    explicit mrb_exception(std::string const& text) : msg(text) {}
    std::string msg;
    char const* what() const noexcept override { return msg.c_str(); }
};
} // namespace mrb

