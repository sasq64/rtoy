
#include <doctest/doctest.h>

#include <mrb/conv.hpp>

#include <mrb/get_args.hpp>

#include <mrb/class.hpp>

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
struct Value
{
    std::shared_ptr<void> ptr;
    mrb_state* mrb;
    mrb_value val{};

    operator mrb_value() const // NOLINT
    {
        return val;
    }

    Value() = default;

    template <typename T>
    Value(mrb_state* _mrb, T* p)
        : mrb{_mrb}, val(mrb::to_value(std::move(p), mrb))
    {
        ptr = std::shared_ptr<void>(mrb_ptr(val), [this](void*) {
            fmt::print("UNREG\n");
            mrb_gc_unregister(mrb, val);
        });
        // Stop value from being garbage collected
        mrb_gc_register(mrb, val);
    }

    template <typename T>
    T as()
    {
        return value_to<T>();
    }

    void clear() { ptr = nullptr; }
};

template <typename T, typename... ARGS>
Value new_obj(mrb_state* mrb, ARGS... args)
{
    return Value(mrb, new T(args...));
}

}; // namespace mrb
struct Person
{
    std::string name;
    int age{};
    static inline mrb::Value instance;
    static inline int counter = 0;
    Person() { counter++; }
    ~Person() { counter--; }
};

TEST_CASE("class")
{
    auto* ruby = mrb_open();

    auto* person_class = mrb::make_class<Person>(ruby, "Person");
    mrb::add_method<Person>(
        ruby, "age=", [](Person* p, int age) { p->age = age; });
    mrb::add_method<Person>(
        ruby, "age", [](Person const* p) { return p->age; });

    mrb::add_method<Person>(ruby, "copy_from", [](Person* p, Person* src) {
        p->name = src->name;
        p->age = src->age;
    });

    mrb::add_method<Person>(
        ruby, "dup", [](Person const* p) { return new Person(*p); });

    auto f = mrb_load_string(ruby,
        "person = Person.new ; person.age = 5 ; other = "
        "Person.new ; other.copy_from(person) ; person.age = 2 ; other.age");
    CHECK(mrb::value_to<int>(f) == 5);
    CHECK(Person::counter == 2);
    mrb_load_string(ruby, "GC.start");
    CHECK(Person::counter == 0);

    mrb_close(ruby);
}

TEST_CASE("retain") {}

#if 0

    auto p = mrb::new_obj<Person>(ruby);
    mrb_define_global_const(ruby, "PERSON", p);
    RUBY_CHECK("PERSON.age == 0");

    // mrb::add_function(ruby, "save", [](Person* person) {
    // });

    f = mrb_load_string(ruby, "person = Person.new ; person.age = 3 ; other = "
                              "person.dup ; p other ; other.age");

    fmt::print("Counter {}\n", Person::counter);
    CHECK(mrb::value_to<int>(f) == 3);

    mrb::add_function(
        ruby, "test_fn", [](mrb::ArgN n, int x, int y, int z, mrb_state* ruby) {
            fmt::print("{} arg {} {} {}\n", n, x, y, z);
            return x + y + z;
        });

    RUBY_CHECK("test_fn(1,2) == 3");

    fmt::print("Counter {}\n", Person::counter);
    auto mv = mrb::new_obj<Person>(ruby);

    Person::instance.clear();
    p.clear();
    fmt::print("Counter {}\n", Person::counter);

    mrb_close(ruby);
    fmt::print("Counter {}\n", Person::counter);

    CHECK(Person::counter == 0);
}
#endif
