#pragma once

#include "conv.hpp"
#include "get_args.hpp"
#include "class.hpp"

extern "C"
{
#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/compile.h>
#include <mruby/data.h>
#include <mruby/proc.h>
#include <mruby/string.h> // NOLINT
#include <mruby/value.h>
#include <mruby/variable.h>
}

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace mrb {

template <class CLASS, class RET, class... ARGS, size_t... A>
RET method(RET (CLASS::*f)(ARGS...), mrb_state* mrb, mrb_value self,
    std::index_sequence<A...> is)
{
    auto* ptr = static_cast<CLASS*>(DATA_PTR(self));
    auto input = get_args<ARGS...>(mrb, nullptr, nullptr, is);
    return (ptr->*f)(std::get<A>(input)...);
}

template <class CLASS, class RET, class... ARGS, size_t... A>
RET method(RET (CLASS::*f)(ARGS...) const, mrb_state* mrb, mrb_value self,
    std::index_sequence<A...> is)
{
    auto* ptr = static_cast<CLASS*>(DATA_PTR(self));
    auto input = get_args<ARGS...>(mrb, nullptr, nullptr, is);
    return (ptr->*f)(std::get<A>(input)...);
}

//! Call method using arguments from mruby
template <class CLASS, class RET, class... ARGS>
RET method(RET (CLASS::*f)(ARGS...), mrb_state* mrb, mrb_value self)
{
    return method(f, mrb, self, std::make_index_sequence<sizeof...(ARGS)>());
}

//! Call method using arguments from mruby
template <class CLASS, class RET, class... ARGS>
RET method(RET (CLASS::*f)(ARGS...) const, mrb_state* mrb, mrb_value self)
{
    return method(f, mrb, self, std::make_index_sequence<sizeof...(ARGS)>());
}

// A 'Data class' is a a use defined class that corresponds to a specific
// `RClass`. They are tightly coupled and normally the data class contains
// a static reference to the corresponding RClass.

// Get corresponding RClass from a data class. Default: The data class must
// have a static member called 'rclass'. Specialize for custom types.
template <typename C>
RClass* get_rclass()
{
    return C::rclass;
}

// Get corresponding mrb_data_type from a data class. Default: The data class
// must have a static member called 'dt'. Specialize for custom types.
//template <typename C>
//mrb_data_type* get_data_type()
//{
//    return &C::dt;
//}

// Create a new instance of a data class `D`
template <typename D>
mrb_value new_data_obj(mrb_state* mrb, D* data)
{
    auto obj = mrb_obj_new(mrb, get_rclass<D>(), 0, nullptr);
    DATA_PTR(obj) = data;
    DATA_TYPE(obj) = get_data_type<D>(mrb);
    return obj;
}

inline std::optional<std::string> check_exception(mrb_state* ruby)
{
    if (ruby->exc != nullptr) {
        auto obj = mrb_funcall(ruby, mrb_obj_value(ruby->exc), "inspect", 0);
        return value_to<std::string>(obj);
    }
    return std::nullopt;
}

inline std::vector<std::string> get_backtrace(mrb_state* ruby)
{
    auto bt = mrb_funcall(ruby, mrb_obj_value(ruby->exc), "backtrace", 0);

    std::vector<std::string> backtrace;
    for (int i = 0; i < ARY_LEN(mrb_ary_ptr(bt)); i++) {
        auto v = mrb_ary_entry(bt, i);
        auto s = mrb_funcall(ruby, v, "to_s", 0);
        std::string line(RSTRING_PTR(s), RSTRING_LEN(s));
        fmt::print("LINE:{}\n", line);
        backtrace.emplace_back(line);
    }
    return backtrace;
}


} // namespace mrb
