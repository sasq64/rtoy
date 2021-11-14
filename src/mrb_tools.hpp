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

struct mrb_exception : public std::exception
{
    explicit mrb_exception(std::string const& text) : msg(text) {}
    std::string msg;
    char const* what() const noexcept override { return msg.c_str(); }
};
template <typename ARG>
constexpr size_t get_spec(std::vector<char>&, std::vector<void*>&, ARG*);

/* template <typename PTR> */
/* constexpr void get_spec(size_t i, PTR*, char* data) */
/* { */
/*     data[i] = 'p'; */
/* } */

template <typename OBJ>
constexpr inline size_t get_spec(
    std::vector<char>& target, std::vector<void*>& ptrs, OBJ** p)
{
    ptrs.push_back(p);
    ptrs.push_back(&OBJ::dt);
    target.push_back('d');
    return target.size();
}

template <>
inline size_t get_spec(
    std::vector<char>& target, std::vector<void*>& ptrs, mrb_int* p)
{
    ptrs.push_back(p);
    target.push_back('i');
    return target.size();
}

template <>
inline size_t get_spec(
    std::vector<char>& target, std::vector<void*>& ptrs, mrb_float* p)
{
    ptrs.push_back(p);
    target.push_back('f');
    return target.size();
}

template <>
inline size_t get_spec(
    std::vector<char>& target, std::vector<void*>& ptrs, const char** p)
{
    ptrs.push_back(p);
    target.push_back('z');
    return target.size();
}

template <>
inline size_t get_spec(
    std::vector<char>& target, std::vector<void*>& ptrs, mrb_value* p)
{
    ptrs.push_back(p);
    target.push_back('o');
    return target.size();
}

template <>
inline size_t get_spec(
    std::vector<char>& target, std::vector<void*>& ptrs, mrb_bool* p)
{
    ptrs.push_back(p);
    target.push_back('b');
    return target.size();
}


template <typename VAL, size_t N>
inline size_t get_spec(
    std::vector<char>& target, std::vector<void*>& ptrs, std::array<VAL, N>* p)
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

template <class... ARGS, size_t... A>
auto get_args(mrb_state* mrb, std::vector<mrb_value>* restv, int* num,
    std::index_sequence<A...>)
{
    // A tuple to store the arguments. Types are converted to mruby
    std::tuple<typename to_mrb<ARGS>::type...> target;


    std::vector<char> v;
    std::vector<void*> arg_ptrs;
    mrb_int n = 0;
    mrb_value* rest{};

    auto arg_count = mrb_get_argc(mrb);
    if (num) { *num = arg_count; }

    // Build spec string, one character per type, plus a trailing '*' to capture
    // remaining arguments
    ((get_spec(v, arg_ptrs, &std::get<A>(target))), ...);
    if (static_cast<int>(v.size()) > arg_count) { v.resize(arg_count); }
    v.push_back('*');
    v.push_back(0);
    arg_ptrs.push_back(&rest);
    arg_ptrs.push_back(&n);

    // fmt::print("ARG: {}\n", v.data());

    mrb_get_args_a(mrb, v.data(), arg_ptrs.data());

    std::tuple<ARGS...> converted{value_to<ARGS>(std::get<A>(target), mrb)...};
    if (n > 0 && restv != nullptr) {
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
        // Check that mrb_value holds a pointer
        assert((mv.w & 7) == 0); // TODO: Use macro
        // Store the mrb_value pointer as a shared_ptr
        ptr = std::shared_ptr<void>(mrb_ptr(mv),
            [mrb](void* t) { mrb_gc_unregister(mrb, mrb_obj_value(t)); });
        // Stop value from being garbage collected
        mrb_gc_register(mrb, mv);
    }

    template <typename T>
    T* as()
    {
        return self_to<T>(mrb_obj_value(ptr.get()));
    }

    void clear() { ptr = nullptr; }
};

using FNP = void (*)();

template <typename CLASS, FNP FN>
void make_attribute(mrb_state* mrb, std::string const& name)
{
    mrb_define_method(mrb, CLASS::rclass, name.c_str(),
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* ptr = mrb::self_to<CLASS>(self);
            FN();
            return mrb_nil_value();
        });
}

class ScriptInterface
{
    mrb_state* ruby;

public:
    explicit ScriptInterface(mrb_state* _ruby) : ruby{_ruby} {}

    template <typename FX, typename RET, typename... ARGS>
    void gf(std::string const& name, FX const& fn, RET (FX::*)(ARGS...) const)
    {
        static FX _fn{fn};
        mrb_define_module_function(
            ruby, ruby->kernel_module, name.c_str(),
            [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
                FX fn{_fn};
                auto args = mrb::get_args<ARGS...>(mrb);
                auto res = std::apply(fn, args);
                return mrb::to_value(res, mrb);
            },
            MRB_ARGS_REQ(sizeof...(ARGS)));
    }

    template <typename FN>
    void global_function(std::string const& name, FN const& fn)
    {
        gf(name, fn, &FN::operator());
    }

    template <typename RET = void>
    RET run(std::string const& code)
    {
        auto* ctx = mrbc_context_new(ruby);
        ctx->capture_errors = TRUE;
        ctx->lineno = 1;

        // auto ai = mrb_gc_arena_save(ruby);

        auto* parser = mrb_parser_new(ruby);
        if (parser == nullptr) { throw mrb_exception("Can't create parser"); }

        parser->s = code.c_str();
        parser->send = code.c_str() + code.length();
        parser->lineno = 1;
        mrb_parser_parse(parser, ctx);

        if (parser->nwarn > 0) {
            char* msg = mrb_locale_from_utf8(parser->warn_buffer[0].message, -1);
            printf("line %d: %s\n", parser->warn_buffer[0].lineno, msg);
            mrb_locale_free(msg);
            return RET{};
        }
        if (parser->nerr > 0) {
            char* msg = mrb_locale_from_utf8(parser->error_buffer[0].message, -1);
            printf("line %d: %s\n", parser->error_buffer[0].lineno, msg);
            mrb_locale_free(msg);
            return RET{};
        }
        struct RProc* proc = mrb_generate_code(ruby, parser);
        mrb_parser_free(parser);
        if (proc == nullptr) { throw mrb_exception("Can't generate code"); }
        // struct RClass* target = ruby->object_class;
        // MRB_PROC_SET_TARGET_CLASS(proc, target);
        mrb_int stack_keep = 0;
        auto result = mrb_vm_run(ruby, proc, mrb_top_self(ruby), stack_keep);
        return value_to<RET>(result);
        // stack_keep = proc->body.irep->nlocals;
        // mrb_gc_arena_restore(ruby, ai);
    }
};

template <typename CLASS>
class ScriptClass
{
    mrb_state* ruby;
public:
    static inline RClass* rclass = nullptr;
    static inline mrb_data_type dt{nullptr, nullptr};

public:
    ScriptClass(mrb_state* _ruby, std::string const& name) : ruby{_ruby}
    {
        rclass = mrb_define_class(ruby, name.c_str(), nullptr);
        MRB_SET_INSTANCE_TT(rclass, MRB_TT_DATA);
        assert(dt.struct_name == nullptr);
        dt = {name.c_str(),
            [](mrb_state*, void* data) { delete static_cast<CLASS*>(data); }};
    }

    template <typename FX, typename RET, typename... ARGS>
    void m(std::string const& name, FX const& fn,
        RET (FX::*)(CLASS*, ARGS...) const)
    {
        static FX _fn{fn};
        mrb_define_method(
            ruby, rclass, name.c_str(),
            [](mrb_state* mrb, mrb_value self) -> mrb_value {
                FX fn{_fn};
                auto args = mrb::get_args<ARGS...>(mrb);
                auto* ptr = mrb::self_to<CLASS>(self);
                if constexpr (std::is_same<RET, void>()) {
                    std::apply(fn, std::tuple_cat(std::make_tuple(ptr), args));
                    return mrb_nil_value();
                } else {
                    auto res = std::apply(
                        fn, std::tuple_cat(std::make_tuple(ptr), args));
                    return mrb::to_value(res, mrb);
                }
            },
            MRB_ARGS_REQ(sizeof...(ARGS)));
    }

    template <typename FN>
    void method(std::string const& name, FN const& fn)
    {
        m(name, fn, &FN::operator());
    }

    template <typename FX, typename... ARGS>
    void i(FX const& fn, void (FX::*)(CLASS*, ARGS...) const)
    {
        static FX _fn{fn};
        mrb_define_method(
            ruby, rclass, "initialize",
            [](mrb_state* mrb, mrb_value self) -> mrb_value {
                FX fn{_fn};
                auto args = mrb::get_args<ARGS...>(mrb);
                auto* cls = new CLASS();
                DATA_PTR(self) = (void*)cls; // NOLINT
                DATA_TYPE(self) = &dt;       // NOLINT
                std::apply(fn, std::tuple_cat(std::make_tuple(cls), args));
                return mrb_nil_value();
            },
            MRB_ARGS_REQ(sizeof...(ARGS)));
    }
    template <typename FN>
    void initialize(FN const& fn)
    {
        i(fn, &FN::operator());
    }
};
} // namespace mrb
