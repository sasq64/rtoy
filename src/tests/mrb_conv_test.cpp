#include <doctest/doctest.h>

#include "../conv.hpp"

using namespace std::string_literals;

void ruby_check(mrb_state* ruby, const char* code, const char* fn, int line)
{
    static bool assert_defined = false;
    if (!assert_defined) {
        mrb_define_module_function(
            ruby, ruby->kernel_module, "assert",
            [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
                mrb_bool what = false;
                const char* fn = nullptr;
                int line = 0;
                mrb_get_args(mrb, "bzi", &what, &fn, &line);
                if (!what) {
                    char temp[1024];
                    sprintf(temp, "\n>> Ruby assertion in %s:%d\n", fn, line);
                    //puts(temp);
                    FAIL(temp);
                }
                return mrb_nil_value();
            },
            MRB_ARGS_REQ(01));
    }

    char temp[1024];
    sprintf(temp, "assert(%s, '%s', %d)", code, fn, line);
    mrb_load_string(ruby, temp);
}

#define RUBY_CHECK(s) ruby_check(ruby, s, __FILE__, __LINE__)

TEST_CASE("mrb conversions")
{
    auto* ruby = mrb_open();
    mrb_value v;

    v = mrb_load_string(ruby, "3");
    CHECK(mrb::value_to<int>(v) == 3);
    CHECK(mrb::value_to<float>(v) == 3.0F);
    CHECK_THROWS(mrb::value_to<std::string>(v));

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

    mrb_define_global_const(ruby, "THREE", mrb::to_value(3));
    RUBY_CHECK("THREE == 2");

    mrb_define_global_const(ruby, "A", mrb::to_value(std::array{5,4,3}));
    RUBY_CHECK("A == [5,4,3]");



}
