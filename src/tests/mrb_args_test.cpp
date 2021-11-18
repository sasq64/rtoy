
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
struct rptr
{
    std::shared_ptr<void> ptr;
    mrb_value val;

    operator mrb_value() const // NOLINT
    {
        return mrb_obj_value(ptr.get());
    }

    explicit operator bool() const { return ptr.operator bool(); }

    rptr() = default;

    rptr(mrb_state* mrb, mrb_value mv) : val(mv)
    {
        // Check that mrb_value holds a pointer
        assert((mv.w & 7) == 0); // TODO: Use macro
        // Store the mrb_value pointer as a shared_ptr
        ptr = std::shared_ptr<void>(mrb_ptr(mv), [mrb](void* t) {
            fmt::print("UNREG\n");
            mrb_gc_unregister(mrb, mrb_obj_value(t));
        });
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

template <typename T>
rptr new_obj(mrb_state* mrb)
{
    auto obj = mrb_obj_new(mrb, get_class<T>(mrb), 0, nullptr);
    return rptr(mrb, obj);
}

}; // namespace mrb
struct Person
{
    std::string name;
    int age{};
    static inline mrb::rptr instance;
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
    mrb::add_method<Person>(ruby, "age", [](Person* p) { return p->age; });

    mrb::add_method<Person>(ruby, "copy_from", [](Person* p, Person* src) {
        p->name = src->name;
        p->age = src->age;
    });

    mrb::add_method<Person>(ruby, "dup", [](Person* p, mrb_state* mrb) {
        auto* np = new Person(*p);
        Person::instance = mrb::rptr(mrb, mrb::to_value(np, mrb));
        return mrb_value(Person::instance);
    });

    auto f = mrb_load_string(ruby,
        "person = Person.new ; person.age = 5 ; other = "
        "Person.new ; other.copy_from(person) ; other.age");
    CHECK(mrb::value_to<int>(f) == 5);

    fmt::print("Counter {}\n", Person::counter);
    mrb_load_string(ruby, "GC.start");
    fmt::print("Counter {}\n", Person::counter);

    auto p = mrb::new_obj<Person>(ruby);
    mrb_define_global_const(ruby, "PERSON", p);
    RUBY_CHECK("PERSON.age == 0");

    // mrb::add_function(ruby, "save", [](Person* person) {
    // });

    f = mrb_load_string(ruby, "person = Person.new ; person.age = 3 ; other = "
                              "person.dup ; p other ; other.age");

    CHECK(mrb::value_to<int>(f) == 3);

    mrb::add_function(
        ruby, "test_fn", [](mrb::ArgN n, int x, int y, int z, mrb_state* ruby) {
            fmt::print("{} arg {} {} {}\n", n,x,y,z);
            return x + y + z;
        });

    RUBY_CHECK("test_fn(1,2) == 3");

    mrb_close(ruby);

    CHECK(Person::counter == 0);
}
