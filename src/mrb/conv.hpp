#pragma once

#include "base.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <vector>

namespace mrb {

template <typename Type>
struct is_std_array : std::false_type
{};

template <typename Item, std::size_t N>
struct is_std_array<std::array<Item, N>> : std::true_type
{};

template <typename Type>
struct is_std_vector : std::false_type
{};

template <typename Item>
struct is_std_vector<std::vector<Item>> : std::true_type
{};

template <typename CLASS>
struct Lookup
{
    static inline std::unordered_map<mrb_state*, RClass*> rclasses;
    static inline std::unordered_map<mrb_state*, mrb_data_type> dts;
};

struct Symbol
{
    Symbol() = default;
    Symbol(mrb_sym s) : sym(s) {}
    Symbol(mrb_state* mrb, std::string const& name) {
        sym = mrb_intern_cstr(mrb, name.c_str());
    }
    // std::string sym;
    mrb_sym sym{};
    // bool operator==(const char* t) const { return std::string(t) == sym; }
    // operator std::string() { return sym; } // NOLINT
    operator uint32_t() { return sym; } // NOLINT
};

//! Convert ruby (mrb_value) type to native
template <typename TARGET>
TARGET value_to(mrb_value obj, mrb_state* mrb = nullptr)
{
    if constexpr (std::is_pointer_v<TARGET>) {
        auto* res = DATA_PTR(obj);
        if (res == nullptr) { throw mrb_exception("nullptr"); }
        return static_cast<TARGET>(res);

    } else if constexpr (is_std_vector<TARGET>()) {
        TARGET result;
        using VAL = typename TARGET::value_type;
        if (!mrb_array_p(obj)) { obj = mrb_funcall(mrb, obj, "to_a", 0); }
        if (mrb_array_p(obj)) {
            int sz = ARY_LEN(mrb_ary_ptr(obj)); // NOLINT
            for (int i = 0; i < sz; i++) {
                auto v = mrb_ary_entry(obj, i);
                result.push_back(value_to<VAL>(v));
            }
        } else {
            mrb_raise(mrb, E_TYPE_ERROR, "not an array");
        }
        return result;
    } else if constexpr (is_std_array<TARGET>()) {
        TARGET result;
        using VAL = typename TARGET::value_type;
        if (!mrb_array_p(obj)) { obj = mrb_funcall(mrb, obj, "to_a", 0); }
        if (mrb_array_p(obj)) {
            int sz = ARY_LEN(mrb_ary_ptr(obj)); // NOLINT
            for (int i = 0; i < static_cast<int>(result.size()); i++) {
                auto v = mrb_ary_entry(obj, i);
                result[i] = i < sz ? value_to<VAL>(v) : VAL{};
            }
        } else {
            mrb_raise(mrb, E_TYPE_ERROR, "not an array");
        }
        return result;
    } else if constexpr (std::is_same_v<TARGET, Symbol>) {
        return Symbol{mrb_symbol(obj)};
    } else if constexpr (std::is_same_v<TARGET, std::string_view>) {
        if (mrb_string_p(obj)) {
            return std::string_view(
                RSTRING_PTR(obj), RSTRING_LEN(obj)); // NOLINT
        }
        throw std::exception();
    } else if constexpr (std::is_same_v<TARGET, std::string>) {
        if (mrb_string_p(obj)) {
            return std::string(RSTRING_PTR(obj), RSTRING_LEN(obj)); // NOLINT
        }
        // TODO: Find real string
        if (mrb_symbol_p(obj)) { return "SYM"; }
        throw std::exception();
    } else if constexpr (std::is_same_v<TARGET, bool>) {
        return mrb_bool(obj);
    } else if constexpr (std::is_arithmetic_v<TARGET>) {
        if (mrb_float_p(obj)) { return static_cast<TARGET>(mrb_float(obj)); }
        if (mrb_fixnum_p(obj)) { return static_cast<TARGET>(mrb_fixnum(obj)); }
        if (mrb_symbol_p(obj)) { return static_cast<TARGET>(mrb_symbol(obj)); }
        throw std::exception();
    } else {
        return static_cast<TARGET>(obj);
        //    throw std::exception();
    }
}

template <typename TARGET>
void copy_value_to(TARGET* target, mrb_value v, mrb_state* mrb)
{
    *target = value_to<TARGET>(v, mrb);
}

//! Convert native type to ruby (mrb_value)
inline mrb_value to_value(const char* r, mrb_state* mrb)
{
    return mrb_str_new_cstr(mrb, r);
}

template <typename RET,
    std::enable_if_t<std::is_pointer<std::remove_reference_t<RET>>::value,
        bool> = true>
mrb_value to_value(RET&& r, mrb_state* const mrb)
{
    // if constexpr (std::is_rvalue_reference_v<decltype(r)>) {
    using T = typename std::remove_pointer_t<std::remove_reference_t<RET>>;
    using LU = Lookup<T>;
    auto* o = mrb_obj_alloc(mrb, MRB_TT_DATA, LU::rclasses[mrb]);
    auto obj = mrb_obj_value(o);
    DATA_PTR(obj) = r;
    DATA_TYPE(obj) = &LU::dts[mrb];
    return obj;
    //} else {
    //    return RET::pointers_must_be_moved;
    //}
}

template <typename RET,
    std::enable_if_t<!std::is_pointer<std::remove_reference_t<RET>>::value,
        bool> = true>
mrb_value to_value(RET const& r, mrb_state* const mrb)
{
    // fmt::print("toval {}\n", typeid(RET).name());
    if constexpr (std::is_same_v<RET, mrb_value>) {
        return r;
    } else if constexpr (std::is_same_v<RET, bool>) {
        return mrb_bool_value(r);
    } else if constexpr (std::is_floating_point_v<RET>) {
        return mrb_float_value(mrb, r);
    } else if constexpr (std::is_integral_v<RET>) {
        return mrb_int_value(mrb, r);
    } else if constexpr (std::is_enum_v<RET>) {
        return mrb_int_value(mrb, r);
    } else if constexpr (std::is_same_v<
                             typename std::remove_reference<RET>::type,
                             std::string>) {
        return mrb_str_new_cstr(mrb, r.c_str());
    } else if constexpr (std::is_same_v<RET, mrb_sym>) {
        return mrb_sym_str(mrb, r);
    } else if constexpr (std::is_convertible_v<RET, mrb_value>) {
        return r;
    } else if constexpr (std::is_same_v<RET, Symbol>) {
        fmt::print("Returning {}\n", r.sym);
        return mrb_symbol_value(r.sym);
        // return mrb_check_intern_cstr(mrb, r.sym.c_str());
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

template <typename T, size_t N>
std::array<T, N> to_array(mrb_value ary, mrb_state* mrb)
{
    std::array<T, N> result{};
    if (!mrb_array_p(ary)) { ary = mrb_funcall(mrb, ary, "to_a", 0); }
    if (mrb_array_p(ary)) {
        auto sz = ARY_LEN(mrb_ary_ptr(ary));
        if (sz != N) {
            mrb_raise(mrb, E_TYPE_ERROR, "not an array");
            return result;
        }
        for (int i = 0; i < sz; i++) {
            auto v = mrb_ary_entry(ary, i);
            result[i] = value_to<T>(v);
        }
    } else {
        mrb_raise(mrb, E_TYPE_ERROR, "not an array");
    }
    return result;
}

inline std::optional<std::string> check_exception(mrb_state* ruby)
{
    if (ruby->exc != nullptr) {
        auto obj = mrb_funcall(ruby, mrb_obj_value(ruby->exc), "inspect", 0);
        return value_to<std::string>(obj);
    }
    return std::nullopt;
}

/*
template <typename T>
std::vector<T> to_vector(mrb_value ary)
{
    auto sz = ARY_LEN(mrb_ary_ptr(ary));
    std::vector<T> result;
    for (int i = 0; i < sz; i++) {
        auto v = mrb_ary_entry(ary, i);
        result.push_back(value_to<T>(v));
    }
    return result;
}
*/

} // namespace mrb
