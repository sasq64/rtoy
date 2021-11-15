#include <doctest/doctest.h>

#include "../conv.hpp"

using namespace std::string_literals;

#define RUBY_CHECK(code) CHECK(mrb::value_to<bool>(mrb_load_string(ruby, code)))

TEST_CASE("value_to")
{
    auto* ruby = mrb_open();
    mrb_value v;

    v = mrb_load_string(ruby, "3");
    CHECK(mrb::value_to<int>(v) == 3);
    CHECK(mrb::value_to<float>(v) == 3.0F);
    CHECK_THROWS(mrb::value_to<std::string>(v));

    auto f = mrb_load_string(ruby, "1 == 2");
    auto t = mrb_load_string(ruby, "3 == 3");
    CHECK(mrb::value_to<bool>(f) == false);
    CHECK(mrb::value_to<bool>(t) == true);

    v = mrb_load_string(ruby, "4.3");
    CHECK(mrb::value_to<float>(v) == 4.3F);
    CHECK(mrb::value_to<int>(v) == 4);

    v = mrb_load_string(ruby, "'hello'");
    CHECK(mrb::value_to<std::string>(v) == "hello");
    CHECK_THROWS(mrb::value_to<int>(v));

    v = mrb_load_string(ruby, "[1,2,3,4]");
    CHECK(mrb::value_to<std::array<int, 4>>(v, ruby) == std::array{1, 2, 3, 4});
    CHECK(mrb::value_to<std::array<int, 2>>(v, ruby) == std::array{1, 2});
    CHECK(mrb::value_to<std::array<int, 6>>(v, ruby) ==
          std::array{1, 2, 3, 4, 0, 0});
    CHECK(mrb::value_to<std::array<float, 3>>(v, ruby) ==
          std::array{1.0F, 2.0F, 3.0F});

    CHECK(mrb::value_to<std::vector<int>>(v, ruby) == std::vector{1, 2, 3, 4});
    CHECK(mrb::value_to<std::vector<float>>(v, ruby) ==
          std::vector{1.0F, 2.0F, 3.0F, 4.0F});

    v = mrb_load_string(ruby, "['a', 'b', 'c']");
    CHECK(mrb::value_to<std::vector<std::string>>(v, ruby) ==
          std::vector{"a"s, "b"s, "c"s});

    mrb_close(ruby);
}

TEST_CASE("to_value")
{
    auto* ruby = mrb_open();
    mrb_define_global_const(ruby, "THREE", mrb::to_value(3));
    RUBY_CHECK("THREE == 3");

    mrb_define_global_const(ruby, "STRING", mrb::to_value("hello", ruby));
    RUBY_CHECK("STRING == 'hello'");

    std::string h = "people";
    mrb_define_global_const(ruby, "STRING", mrb::to_value(h, ruby));
    RUBY_CHECK("STRING == 'people'");

    mrb_define_global_const(
        ruby, "A", mrb::to_value(std::array{5, 4, 3}, ruby));
    RUBY_CHECK("A == [5,4,3]");

    std::vector<std::string> names;
    names.emplace_back("Alex");
    names.emplace_back("Bjarne");
    mrb_define_global_const(
        ruby, "NAMES", mrb::to_value(names, ruby));
    RUBY_CHECK("NAMES == ['Alex', 'Bjarne']");

    mrb_close(ruby);
}
