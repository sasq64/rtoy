#pragma once

extern "C"
{
#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/compile.h>
#include <mruby/data.h>
#include <mruby/proc.h>
#include <mruby/string.h>
#include <mruby/value.h>
#include <mruby/variable.h>
}

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace mrb {

template <typename Target>
Target* self_to(mrb_value self)
{
    return static_cast<Target*>(
        (static_cast<struct RData*>(mrb_ptr(self)))->data);
}

template <typename ARG>
constexpr void get_spec(size_t i, ARG, char* data);

template <typename PTR>
constexpr void get_spec(size_t i, PTR*, char* data)
{
    data[i] = 'p';
}

template <>
constexpr void get_spec(size_t i, mrb_int, char* data)
{
    data[i] = 'i';
}

template <>
constexpr void get_spec(size_t i, mrb_float, char* data)
{
    data[i] = 'f';
}

template <>
inline void get_spec(size_t i, const char*, char* data)
{
    data[i] = 'z';
}

template <>
inline void get_spec(size_t i, mrb_value, char* data)
{
    data[i] = 'o';
}

template <typename T>
struct ToMrb
{
    using To = T;
};

template <>
struct ToMrb<std::string>
{
    using To = const char*;
};

template <>
struct ToMrb<std::string const&>
{
    using To = const char*;
};

template <>
struct ToMrb<float>
{
    using To = mrb_float;
};

template <>
struct ToMrb<int>
{
    using To = mrb_int;
};

template <>
struct ToMrb<unsigned int>
{
    using To = mrb_int;
};

template <class... ARGS, size_t... A>
auto get_args(
    mrb_state* mrb, std::vector<mrb_value>* restv, std::index_sequence<A...>)
{
    std::tuple<typename ToMrb<ARGS>::To...> input;

    std::array<char, sizeof...(ARGS) + 2> spec;
    spec[sizeof...(ARGS)] = '*';
    spec[sizeof...(ARGS) + 1] = 0;
    (void)std::tuple{(get_spec(A, std::get<A>(input), spec.data()), 0)...};

    mrb_int n = 0;
    mrb_value* rest{};
    mrb_get_args(mrb, spec.data(), &std::get<A>(input)..., &rest, &n);
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            restv->push_back(rest[i]);
        }
    }
    return input;
}

//! Get function args according to type list
//! ex
//!
//! auto [x,y, txt] = get_args<float, float, std::string>(mrb);
//! draw_text(x,y, txt);
//
template <class... ARGS>
auto get_args(mrb_state* mrb)
{
    return get_args<ARGS...>(
        mrb, nullptr, std::make_index_sequence<sizeof...(ARGS)>());
}

template <class... ARGS>
auto get_args(mrb_state* mrb, std::vector<mrb_value>& rest)
{
    return get_args<ARGS...>(
        mrb, &rest, std::make_index_sequence<sizeof...(ARGS)>());
}

//! Convert native type to ruby (mrb_value)
template <typename RET>
mrb_value to_value(RET const& r, mrb_state* const mrb)
{
    if constexpr (std::is_floating_point_v<RET>) {
        return mrb_float_value(mrb, r);
    } else if constexpr (std::is_integral_v<RET>) {
        return mrb_fixnum_value(r);
    } else if constexpr (std::is_same_v<RET, std::string>) {
        return mrb_str_new_cstr(mrb, r.c_str());
    } else if constexpr (std::is_same_v<RET, const char*>) {
        return mrb_str_new_cstr(mrb, r);
    } else if constexpr (std::is_same_v<RET, mrb_sym>) {
        return mrb_sym_str(mrb, r);
    } else {
        return RET::can_not_convert;
    }
}

template <typename ELEM>
mrb_value to_value(std::vector<ELEM> const& r, mrb_state* mrb)
{
    std::vector<mrb_value> output(r.size());
    std::transform(r.begin(), r.end(), output.begin(),
        [&](ELEM const& e) { return to_value(e, mrb); });
    return mrb_ary_new_from_values(mrb, output.size(), output.data());
}

template <typename ELEM, size_t N>
mrb_value to_value(std::array<ELEM, N> const& r, mrb_state* mrb)
{
    std::vector<mrb_value> output(r.size());
    std::transform(r.begin(), r.end(), output.begin(),
        [&](ELEM const& e) { return to_value(e, mrb); });
    return mrb_ary_new_from_values(mrb, output.size(), output.data());
}

//! Convert ruby (mrb_value) type to native
template <typename TARGET>
TARGET to(mrb_value obj)
{
    if constexpr (std::is_same_v<TARGET, std::string_view>) {
        return std::string_view(RSTRING_PTR(obj), RSTRING_LEN(obj)); // NOLINT
    } else if constexpr (std::is_same_v<TARGET, std::string>) {
        return std::string(RSTRING_PTR(obj), RSTRING_LEN(obj)); // NOLINT
    } else if constexpr (std::is_floating_point_v<TARGET>) {
        return mrb_float(obj);
    } else if constexpr (std::is_integral_v<TARGET>) {
        return mrb_fixnum(obj);
    } else {
        throw std::exception();
    }
}

template <class CLASS, class RET, class... ARGS, size_t... A>
RET method(RET (CLASS::*f)(ARGS...), mrb_state* mrb, mrb_value self,
    std::index_sequence<A...> is)
{
    auto* ptr = (CLASS*)DATA_PTR(self);
    auto input = get_args<ARGS...>(mrb, nullptr, is);
    return (ptr->*f)(std::get<A>(input)...);
}

template <class CLASS, class RET, class... ARGS, size_t... A>
RET method(RET (CLASS::*f)(ARGS...) const, mrb_state* mrb, mrb_value self,
    std::index_sequence<A...> is)
{
    auto* ptr = (CLASS*)DATA_PTR(self);
    auto input = get_args<ARGS...>(mrb, nullptr, is);
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
template <typename C>
mrb_data_type* get_data_type()
{
    return &C::dt;
}
// Create a new instance of a data class `D`
template <typename D>
mrb_value new_data_obj(mrb_state* mrb, D* data)
{
    auto obj = mrb_obj_new(mrb, get_rclass<D>(), 0, nullptr);
    DATA_PTR(obj) = data;
    DATA_TYPE(obj) = get_data_type<D>();
    return obj;
}

inline std::optional<std::string> check_exception(mrb_state* ruby)
{
    if (ruby->exc != nullptr) {
        auto obj = mrb_funcall(ruby, mrb_obj_value(ruby->exc), "inspect", 0);
        return to<std::string>(obj);
    }
    return std::nullopt;
}

} // namespace mrb
