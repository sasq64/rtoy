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
#include <cassert>
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

template <>
inline void get_spec(size_t i, mrb_bool, char* data)
{
    data[i] = 'b';
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
    // A tuple to store the arguments. Types are converted to mruby
    std::tuple<typename ToMrb<ARGS>::To...> input;

    // Build spec string, one character per type, plus a trailing '*' to capture
    // remaining arguments
    std::array<char, sizeof...(ARGS) + 2> spec;
    spec[sizeof...(ARGS)] = '*';
    spec[sizeof...(ARGS) + 1] = 0;
    (void)std::tuple{(get_spec(A, std::get<A>(input), spec.data()), 0)...};

    mrb_int n = 0;
    mrb_value* rest{};
    mrb_get_args(mrb, spec.data(), &std::get<A>(input)..., &rest, &n);
    std::tuple<ARGS...> converted{static_cast<ARGS>(std::get<A>(input))...};
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            restv->push_back(rest[i]);
        }
    }
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
    if constexpr (std::is_same_v<RET, mrb_value>) {
        return r;
    } else if constexpr (std::is_same_v<RET, bool>) {
        return mrb_bool_value(r);
    } else if constexpr (std::is_floating_point_v<RET>) {
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
    return mrb_ary_new_from_values(
        mrb, static_cast<mrb_int>(output.size()), output.data());
}

template <typename ELEM, size_t N>
mrb_value to_value(std::array<ELEM, N> const& r, mrb_state* mrb)
{
    std::vector<mrb_value> output(r.size());
    std::transform(r.begin(), r.end(), output.begin(),
        [&](ELEM const& e) { return to_value(e, mrb); });
    return mrb_ary_new_from_values(
        mrb, static_cast<mrb_int>(output.size()), output.data());
}

//! Convert ruby (mrb_value) type to native
template <typename TARGET>
TARGET to(mrb_value obj)
{
    if constexpr (std::is_same_v<TARGET, std::string_view>) {
        return std::string_view(RSTRING_PTR(obj), RSTRING_LEN(obj)); // NOLINT
    } else if constexpr (std::is_same_v<TARGET, std::string>) {
        return std::string(RSTRING_PTR(obj), RSTRING_LEN(obj)); // NOLINT
    } else if constexpr (std::is_arithmetic_v<TARGET>) {
        if (mrb_float_p(obj)) { return static_cast<TARGET>(mrb_float(obj)); }
        if (mrb_fixnum_p(obj)) { return static_cast<TARGET>(mrb_fixnum(obj)); }
        throw std::exception();
    } else {
        throw std::exception();
    }
}

template <typename T, size_t N>
std::array<T, N> to_array(mrb_value ary)
{
    std::array<T, N> result{};
    if (mrb_array_p(ary)) {
        auto sz = ARY_LEN(mrb_ary_ptr(ary));
        for (int i = 0; i < sz; i++) {
            auto v = mrb_ary_entry(ary, i);
            result[i] = to<T>(v);
        }
    } else {
        // TODO: Exception
    }
    return result;
}

template <typename T>
std::vector<T> to_vector(mrb_value ary)
{
    auto sz = ARY_LEN(mrb_ary_ptr(ary));
    std::vector<T> result;
    for (int i = 0; i < sz; i++) {
        auto v = mrb_ary_entry(ary, i);
        result.push_back(to<T>(v));
    }
    return result;
}

template <class CLASS, class RET, class... ARGS, size_t... A>
RET method(RET (CLASS::*f)(ARGS...), mrb_state* mrb, mrb_value self,
    std::index_sequence<A...> is)
{
    auto* ptr = static_cast<CLASS*>(DATA_PTR(self));
    auto input = get_args<ARGS...>(mrb, nullptr, is);
    return (ptr->*f)(std::get<A>(input)...);
}

template <class CLASS, class RET, class... ARGS, size_t... A>
RET method(RET (CLASS::*f)(ARGS...) const, mrb_state* mrb, mrb_value self,
    std::index_sequence<A...> is)
{
    auto* ptr = static_cast<CLASS*>(DATA_PTR(self));
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

// Hold an mrb_value referencing an object on the native side.
// As long as we have a reference (via the shared_ptr) it will
// not be garbage collected.
struct RubyPtr
{
    std::shared_ptr<void> ptr;

    operator mrb_value() const // NOLINT
    {
        return mrb_obj_value(ptr.get());
    }

    explicit operator bool() const { return ptr.operator bool(); }

    RubyPtr() = default;

    RubyPtr(mrb_state* mrb, mrb_value mv)
    {
        assert((mv.w & 7) == 0); // TODO: Use macro
        ptr = std::shared_ptr<void>(mrb_ptr(mv),
            [mrb](void* t) { mrb_gc_unregister(mrb, mrb_obj_value(t)); });
        mrb_gc_register(mrb, mv);
    }

    template <typename T>
    T* as()
    {
        return self_to<T>(mrb_obj_value(ptr.get()));
    }

    void clear() { ptr = nullptr; }
};

} // namespace mrb
