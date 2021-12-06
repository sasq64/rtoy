
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
template <typename T, typename... ARGS>
Value new_obj(mrb_state* mrb, ARGS... args)
{
    return Value(mrb, new T(args...));
}

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
            //fmt::print("Initialize\n");
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
    if (parent == nullptr) { parent = mrb->object_class; }
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
void add_class_method(mrb_state* ruby, std::string const& name, FX const& fn,
    RET (FX::*)(ARGS...) const)
{
    static FX _fn{fn};
    mrb_define_class_method(
        ruby, Lookup<CLASS>::rclasses[ruby], name.c_str(),
        [](mrb_state* mrb, mrb_value) -> mrb_value {
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

template <typename CLASS, typename FN>
void add_class_method(mrb_state* ruby, std::string const& name, FN const& fn)
{
    add_class_method<CLASS>(ruby, name, fn, &FN::operator());
}

template <typename CLASS, typename SELF, typename FX, typename RET, typename... ARGS>
void add_method(mrb_state* ruby, std::string const& name, FX const& fn,
    RET (FX::*)(SELF, ARGS...) const)
{
    static FX _fn{fn};
    mrb_define_method(
        ruby, Lookup<CLASS>::rclasses[ruby], name.c_str(),
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            FX fn{_fn};
            auto args = mrb::get_args<ARGS...>(mrb);
            auto ptr = mrb::self_to<SELF>(self);
            if constexpr (std::is_same<RET, void>()) {
                std::apply(fn, std::tuple_cat(std::make_tuple(ptr), args));
                return mrb_nil_value();
            } else {
                return mrb::to_value(
                    std::apply(fn, std::tuple_cat(std::make_tuple(ptr), args)), mrb);
            }
        },
        MRB_ARGS_REQ(sizeof...(ARGS)));
}

template <typename CLASS, typename FN>
void add_method(mrb_state* ruby, std::string const& name, FN const& fn)
{
    add_method<CLASS>(ruby, name, fn, &FN::operator());
}

template <auto PTR, typename CLASS, typename M, typename... ARGS>
void add_method2(
    mrb_state* ruby, std::string const& name, M (CLASS::*)(ARGS...) const)
{
    add_method<CLASS>(ruby, name, [](CLASS* c, ARGS... args) {
        // return c->*PTR(args...);
        return std::invoke(PTR, c, args...);
    });
}

template <auto PTR, typename CLASS, typename M, typename... ARGS>
void add_method2(
    mrb_state* ruby, std::string const& name, M (CLASS::*)(ARGS...))
{
    add_method<CLASS>(ruby, name, [](CLASS* c, ARGS... args) {
        // return c->*PTR(args...);
        return std::invoke(PTR, c, args...);
    });
}


template <auto PTR>
void add_method(mrb_state* ruby, std::string const& name)
{
    add_method2<PTR>(ruby, name, PTR);
}

template <auto PTR, typename CLASS, typename M>
void attr_accessor(mrb_state* ruby, std::string const& name, M CLASS::*)
{
    add_method<CLASS>(ruby, name, [](CLASS* c) { return c->*PTR; });
    add_method<CLASS>(ruby, name + "=", [](CLASS* c, M v) { c->*PTR = v; });
}

template <auto PTR>
void attr_accessor(mrb_state* ruby, std::string const& name)
{
    attr_accessor<PTR>(ruby, name, PTR);
}

template <auto PTR, typename CLASS, typename M>
void attr_reader(mrb_state* ruby, std::string const& name, M CLASS::*)
{
    add_method<CLASS>(ruby, name, [](CLASS* c) { return c->*PTR; });
}

template <auto PTR>
void attr_reader(mrb_state* ruby, std::string const& name)
{
    attr_reader<PTR>(ruby, name, PTR);
}

} // namespace mrb
