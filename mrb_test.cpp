#include "src/mrb_tools.hpp"

int main()
{
    mrb::ScriptInterface ruby { mrb_open() };

    ruby.global_function("test", [](int x) {
        return x + 1;
    });

    ruby.global_function("test2", [](std::string p) {
      return "hello " + p;
    });

    auto script = R"(
        test(1)
    )";

    auto res = ruby.run<int>(script);
    fmt::print("RES {}\n", res);

    auto res2 = ruby.run<std::string>("test2('cool')");
    fmt::print("RES {}\n", res2);
//    ruby.get("$res");

    return 0;

}
