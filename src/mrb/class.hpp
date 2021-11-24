
#pragma once

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
#include <string>
#include <tuple>
#include <vector>

namespace mrb {
template <typename CLASS>
constexpr const char* class_name()
{
    return CLASS::class_name();
}

template <typename T>
RClass* make_class(mrb_state* mrb, const char* name = class_name<T>(),
    RClass* parent = nullptr)
{
    if (parent == nullptr) { parent = mrb->object_class; }
    auto* rclass = mrb_define_class(mrb, name, parent);
    Lookup<T>::rclasses[mrb] = rclass;
    Lookup<T>::dts[mrb] = {
        name, [](mrb_state*, void* data) { delete static_cast<T*>(data); }};
    MRB_SET_INSTANCE_TT(rclass, MRB_TT_DATA);
    mrb_define_method(
        mrb, rclass, "initialize",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* cls = new T();
            DATA_PTR(self) = (void*)cls;            // NOLINT
            DATA_TYPE(self) = &Lookup<T>::dts[mrb]; // NOLINT
            return mrb_nil_value();
        },
        MRB_ARGS_NONE());
    return rclass;
}

template <typename T>
RClass* make_noinit_class(mrb_state* mrb, const char* name = class_name<T>(),
    RClass* parent = nullptr)
{
    auto* rclass = mrb_define_class(mrb, name, parent);
    Lookup<T>::rclasses[mrb] = rclass;
    Lookup<T>::dts[mrb] = {
        name, [](mrb_state*, void* data) { delete static_cast<T*>(data); }};
    MRB_SET_INSTANCE_TT(rclass, MRB_TT_DATA);
    return rclass;
}

template <typename T, typename FN>
void set_deleter(mrb_state* mrb, FN const& f)
{
    Lookup<T>::dts[mrb].dfree = f;
}

template <typename T>
mrb_data_type* get_data_type(mrb_state* mrb)
{
    return &Lookup<T>::dts[mrb];
}

template <typename T>
RClass* get_class(mrb_state* mrb)
{
    return Lookup<T>::rclasses[mrb];
}

template <typename T>
mrb_value new_data_obj(mrb_state* mrb)
{
    return mrb_obj_new(mrb, Lookup<T>::rclasses[mrb], 0, nullptr);
}

template <typename CLASS, typename FX, typename RET, typename... ARGS>
void add_method(mrb_state* ruby, std::string const& name, FX const& fn,
    RET (FX::*)(CLASS*, ARGS...) const)
{
    static FX _fn{fn};
    mrb_define_method(
        ruby, Lookup<CLASS>::rclasses[ruby], name.c_str(),
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            FX fn{_fn};
            auto args = mrb::get_args<ARGS...>(mrb);
            auto* ptr = mrb::self_to<CLASS>(self);
            if constexpr (std::is_same<RET, void>()) {
                std::apply(fn, std::tuple_cat(std::make_tuple(ptr), args));
                return mrb_nil_value();
            } else {
                auto res =
                    std::apply(fn, std::tuple_cat(std::make_tuple(ptr), args));
                return mrb::to_value(res, mrb);
            }
        },
        MRB_ARGS_REQ(sizeof...(ARGS)));
}

template <typename CLASS, typename FN>
void add_method(mrb_state* ruby, std::string const& name, FN const& fn)
{
    add_method<CLASS>(ruby, name, fn, &FN::operator());
}

template <typename CLASS, typename FX, typename RET, typename... ARGS>
void add_method_n(mrb_state* ruby, std::string const& name, FX const& fn,
    RET (FX::*)(CLASS*, int, ARGS...) const)
{
    static FX _fn{fn};
    mrb_define_method(
        ruby, Lookup<CLASS>::rclasses[ruby], name.c_str(),
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            FX fn{_fn};
            auto args = mrb::get_args_n<ARGS...>(mrb);
            auto* ptr = mrb::self_to<CLASS>(self);
            if constexpr (std::is_same<RET, void>()) {
                std::apply(fn, std::tuple_cat(std::make_tuple(ptr), args));
                return mrb_nil_value();
            } else {
                auto res =
                    std::apply(fn, std::tuple_cat(std::make_tuple(ptr), args));
                return mrb::to_value(res, mrb);
            }
        },
        MRB_ARGS_REQ(sizeof...(ARGS)));
}

template <typename CLASS, typename FN>
void add_method_n(mrb_state* ruby, std::string const& name, FN const& fn)
{
    add_method_n<CLASS>(ruby, name, fn, &FN::operator());
}

} // namespace mrb
