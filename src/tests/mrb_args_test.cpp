
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

    auto other_age = mrb_load_string(ruby,
        "person = Person.new ; person.age = 5 ; other = "
        "Person.new ; other.copy_from(person) ; person.age = 2 ; other.age");
    CHECK(mrb::value_to<int>(other_age) == 5);

    CHECK(Person::counter == 2);
    mrb_load_string(ruby, "GC.start");
    CHECK(Person::counter == 0);

    mrb_close(ruby);
}

TEST_CASE("symbols")
{
    auto* ruby = mrb_open();
    auto sym = mrb_intern_cstr(ruby, "testing");
    mrb_define_global_const(ruby, "TEST", mrb_symbol_value(sym));
    mrb_load_string(ruby, "p TEST.class ; p TEST ; TEST = :testing ; p TEST"); 

    //mrb_define_global_const(ruby, "TEST", mrb_check_intern_cstr(ruby, "testing"));
    //  mrb_load_string(ruby, "p TEST.class ; p TEST ; TEST = :testing ; p TEST"); 


    //RUBY_CHECK("TEST == :testing");
    //mrb::Symbol sym { ruby, "testing"};
}

TEST_CASE("shared_ptr")
{
    bool deleter_called = false;
    auto p = std::shared_ptr<void>(nullptr, [&](void*) {
            deleter_called = true;});
    auto p2 = p;
    p = nullptr;
    CHECK(!deleter_called);
    p2 = nullptr;
    CHECK(deleter_called);

}

TEST_CASE("retain") {

    auto* ruby = mrb_open();
    auto* person_class = mrb::make_class<Person>(ruby, "Person");
    mrb::add_method<Person>(
        ruby, "age", [](Person const* p) { return 99; });
    
    auto f = mrb_load_string(ruby, "p = Person.new() ; p.age");
    CHECK(Person::counter == 1);
    mrb_load_string(ruby, "GC.start");
    CHECK(Person::counter == 0);

    mrb::Value v { ruby, new Person() };
    CHECK(Person::counter == 1);
    //mrb_load_string(ruby, "p = Person.new() ; p.age");
    //CHECK(Person::counter == 2);
    //mrb_load_string(ruby, "GC.start");
    //CHECK(Person::counter == 1);

//mrb_define_global_const(ruby, "PERSON", v);
  //      RUBY_CHECK("PERSON.age == 99");

    v.clear();
    mrb_load_string(ruby, "GC.start ; 3");
    CHECK(Person::counter == 1);
    mrb_close(ruby);
    fmt::print("Closed\n");
    CHECK(Person::counter == 0);


}

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
