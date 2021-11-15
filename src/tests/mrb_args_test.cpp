
#include <doctest/doctest.h>

#include "../get_args.hpp"

using namespace std::string_literals;

#define RUBY_CHECK(code) CHECK(mrb::value_to<bool>(mrb_load_string(ruby, code)))

TEST_CASE("get_args")
{
    auto* ruby = mrb_open();

    mrb_define_module_function(
        ruby, ruby->kernel_module, "test",
        [](mrb_state* mrb, mrb_value) -> mrb_value {
            auto [b, s, f] = mrb::get_args<bool, std::string, float>(mrb);
            return mrb::to_value<std::string>(
                fmt::format("{}/{}/{}", b, s, f), mrb);
        },
        MRB_ARGS_REQ(3));

    RUBY_CHECK("test(false, 'hello', 3.14) == 'false/hello/3.14'");
}

namespace mrb {
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
    RClass* rclass = nullptr;
    mrb_data_type dt = {
        Name, [](mrb_state*, void* data) { delete static_cast<CLASS*>(data); }};

    explicit ScriptClass(mrb_state* _ruby, std::string const& name)
        : ruby{_ruby}
    {
        rclass = mrb_define_class(ruby, name.c_str(), nullptr);
        MRB_SET_INSTANCE_TT(rclass, MRB_TT_DATA);
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


template <typename CLASS>
struct Lookup
{
    static mrb_state* ruby;
    static inline RClass* rclass = nullptr;
    static inline mrb_data_type dt = {
        nullptr, [](mrb_state*, void* data) { delete static_cast<CLASS*>(data); }};
};


template <typename T>
RClass* make_class(mrb_state* mrb)
{
    Lookup<T>::rclass = mrb_define_class(mrb, class_name<T>(), nullptr);
    MRB_SET_INSTANCE_TT(Lookup<T>::rclass, MRB_TT_DATA);
    return Lookup<T>::rclass;

}

template <typename CLASS, typename FX, typename RET, typename... ARGS>
void m(std::string const& name, FX const& fn,
    RET (FX::*)(CLASS*, ARGS...) const)
{
    static FX _fn{fn};
    mrb_define_method(
        Lookup<CLASS>::ruby, Lookup<CLASS>::rclass, name.c_str(),
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

template <typename CLASS, typename FN>
void add_method(std::string const& name, FN const& fn)
{
    m<CLASS>(name, fn, &FN::operator());
}




} // namespace mrb

TEST_CASE("class")
{
    auto* ruby = mrb_open();

}

