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


#if 0
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
            char* msg =
                mrb_locale_from_utf8(parser->warn_buffer[0].message, -1);
            printf("line %d: %s\n", parser->warn_buffer[0].lineno, msg);
            mrb_locale_free(msg);
            return RET{};
        }
        if (parser->nerr > 0) {
            char* msg =
                mrb_locale_from_utf8(parser->error_buffer[0].message, -1);
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
constexpr const char* class_name()
{
    return CLASS::class_name();
}

template <typename CLASS>
class ScriptClass
{
    mrb_state* ruby;

    static constexpr const char* Name = class_name<CLASS>();

public:
    static inline RClass* rclass = nullptr;
    static inline mrb_data_type dt = {
        Name, [](mrb_state*, void* data) { delete static_cast<CLASS*>(data); }};

    explicit ScriptClass(mrb_state* _ruby) : ruby{_ruby}
    {
        rclass = mrb_define_class(ruby, Name, nullptr);
        MRB_SET_INSTANCE_TT(rclass, MRB_TT_DATA);
        assert(dt.struct_name == nullptr);
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

#endif

} // namespace mrb
