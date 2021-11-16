
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

}; // namespace mrb
struct Person
{
    std::string name;
    int age{};
};

TEST_CASE("class")
{
    auto* ruby = mrb_open();

    auto* person_class = mrb::make_class<Person>(ruby, "Person");
    mrb::add_method<Person>(
        ruby, "age=", [](Person* p, int age) { p->age = age; });
    mrb::add_method<Person>(ruby, "age", [](Person* p) { return p->age; });

    auto f = mrb_load_string(
        ruby, "person = Person.new ; person.age = 5 ; person.age");

    // Creates an mrb_value referencing Person.
    // Does gc_register to prevent collection
    // When obj goes out of scope on native side we unregister
    // Means gc can collect and free

    CHECK(mrb::value_to<int>(f) == 5);
}

