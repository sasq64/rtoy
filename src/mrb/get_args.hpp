
/* template <typename PTR> */
/* constexpr void get_spec(mrb_state* mrb, size_t i, PTR*, char* data) */
/* { */
/*     data[i] = 'p'; */
/* } */
#pragma once

#include "conv.hpp"

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

struct ArgN
{
    int n;
    operator int() const { return n; }
};

template <typename ARG>
constexpr size_t get_spec(
    mrb_state* mrb, std::vector<char>&, std::vector<void*>&, ARG*);

template <>
constexpr inline size_t get_spec(
    mrb_state* mrb, std::vector<char>& target, std::vector<void*>&, mrb_state**)
{
    // Skip mrb_state, we provide it ourselves
    return 0; // target.size();
}

template <>
constexpr inline size_t get_spec(
    mrb_state* mrb, std::vector<char>& target, std::vector<void*>&, ArgN*)
{
    return 0; // target.size();
}

template <typename OBJ>
constexpr inline size_t get_spec(mrb_state* mrb, std::vector<char>& target,
    std::vector<void*>& ptrs, OBJ** p)
{
    ptrs.push_back(p);
    ptrs.push_back(&Lookup<OBJ>::dts[mrb]);
    // ptrs.push_back(&OBJ::dt);
    target.push_back('d');
    return target.size();
}

template <>
inline size_t get_spec(mrb_state* mrb, std::vector<char>& target,
    std::vector<void*>& ptrs, mrb_int* p)
{
    ptrs.push_back(p);
    target.push_back('i');
    return target.size();
}

template <>
inline size_t get_spec(mrb_state* mrb, std::vector<char>& target,
    std::vector<void*>& ptrs, mrb_float* p)
{
    ptrs.push_back(p);
    target.push_back('f');
    return target.size();
}

template <>
inline size_t get_spec(mrb_state* mrb, std::vector<char>& target,
    std::vector<void*>& ptrs, const char** p)
{
    ptrs.push_back(p);
    target.push_back('z');
    return target.size();
}

template <>
inline size_t get_spec(mrb_state* mrb, std::vector<char>& target,
    std::vector<void*>& ptrs, mrb_value* p)
{
    ptrs.push_back(p);
    target.push_back('o');
    return target.size();
}

template <>
inline size_t get_spec(mrb_state* mrb, std::vector<char>& target,
    std::vector<void*>& ptrs, mrb_bool* p)
{
    ptrs.push_back(p);
    target.push_back('b');
    return target.size();
}

template <typename VAL, size_t N>
inline size_t get_spec(mrb_state* mrb, std::vector<char>& target,
    std::vector<void*>& ptrs, std::array<VAL, N>* p)
{
    fmt::print("#### NOT HERE\n");
    ptrs.push_back(p);
    target.push_back('o');
    return target.size();
}

template <typename T>
struct to_mrb
{
    using type = T;
};

template <>
struct to_mrb<std::string>
{
    using type = const char*;
};

template <>
struct to_mrb<std::string const&>
{
    using type = const char*;
};

template <>
struct to_mrb<float>
{
    using type = mrb_float;
};

template <>
struct to_mrb<int>
{
    using type = mrb_int;
};

template <>
struct to_mrb<unsigned int>
{
    using type = mrb_int;
};

template <typename T, size_t N>
struct to_mrb<std::array<T, N>>
{
    using type = mrb_value;
};

template <typename TARGET, typename SOURCE>
TARGET mrb_to(SOURCE const& s, mrb_state* mrb)
{
    if constexpr (std::is_same_v<mrb_state*, SOURCE>) {
        return mrb;
    } else if constexpr (std::is_same_v<ArgN, SOURCE>) {
        return {mrb_get_argc(mrb)};
    } else if constexpr (std::is_same_v<mrb_value, SOURCE>) {
        return value_to<TARGET>(s, mrb);
    } else {
        return static_cast<TARGET>(s);
    }
}

template <typename Target>
Target* self_to(mrb_value self)
{
    return static_cast<Target*>(
        (static_cast<struct RData*>(mrb_ptr(self)))->data);
}

template <class... ARGS, size_t... A>
auto get_args(mrb_state* mrb, std::vector<mrb_value>* restv, int* num,
    std::index_sequence<A...>)
{
    // A tuple to store the arguments. Types are converted to corresponding
    // mruby types
    std::tuple<typename to_mrb<ARGS>::type...> target;

    std::vector<char> v;
    std::vector<void*> arg_ptrs;
    mrb_int n = 0;
    mrb_value* rest{};

    auto arg_count = mrb_get_argc(mrb);
    if (num) { *num = arg_count; }

    // Build spec string, one character per type, plus a trailing '*' to capture
    // remaining arguments
    ((get_spec(mrb, v, arg_ptrs, &std::get<A>(target))), ...);
    std::string x{v.begin(), v.end()};
    fmt::print("SIZE {} {} {}\n", x, v.size(), arg_count);
    // TODO: If we cap format string we cant add rest args to arg_ptrs
    if (static_cast<int>(v.size()) > arg_count) { v.resize(arg_count); }
    //v.push_back('*');
    v.push_back(0);
    fmt::print("SPEC {} ({})\n", v.data(), sizeof...(ARGS));
    //arg_ptrs.push_back(&rest);
    //arg_ptrs.push_back(&n);

    // fmt::print("ARG: {}\n", v.data());

    mrb_get_args_a(mrb, v.data(), arg_ptrs.data());

    std::tuple<ARGS...> converted{mrb_to<ARGS>(std::get<A>(target), mrb)...};

    //if (n > 0 && restv != nullptr) {
    //    for (int i = 0; i < n; i++) {
    //        restv->push_back(rest[i]);
    //    }
   // }

    return converted;
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
        mrb, nullptr, nullptr, std::make_index_sequence<sizeof...(ARGS)>());
}

template <class... ARGS>
auto get_args_n(mrb_state* mrb)
{
    int n = 0;
    auto res = get_args<ARGS...>(
        mrb, nullptr, &n, std::make_index_sequence<sizeof...(ARGS)>());
    return std::tuple_cat(std::make_tuple(n), res);
}

template <class... ARGS>
auto get_args(mrb_state* mrb, std::vector<mrb_value>& rest)
{
    return get_args<ARGS...>(
        mrb, &rest, nullptr, std::make_index_sequence<sizeof...(ARGS)>());
}
template <typename FX, typename RET, typename... ARGS>
void gf(mrb_state* ruby, std::string const& name, FX const& fn,
    RET (FX::*)(ARGS...) const)
{
    static FX _fn{fn};
    mrb_define_module_function(
        ruby, ruby->kernel_module, name.c_str(),
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            FX fn{_fn};
            auto args = mrb::get_args<ARGS...>(mrb);
            if constexpr (std::is_same<RET, void>()) {
                std::apply(fn, args);
                return mrb_nil_value();
            } else {
                auto res = std::apply(fn, args);
                return mrb::to_value(res, mrb);
            }
        },
        MRB_ARGS_REQ(sizeof...(ARGS)));
}

template <typename FN>
void add_function(mrb_state* ruby, std::string const& name, FN const& fn)
{
    gf(ruby, name, fn, &FN::operator());
}

} // namespace mrb
