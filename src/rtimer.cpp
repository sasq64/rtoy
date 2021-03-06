#include "rtimer.hpp"

#include <fmt/format.h>

#include "error.hpp"
#include "mrb_tools.hpp"
#include <mruby/object.h>

using namespace std::chrono_literals;
using clk = std::chrono::steady_clock;

double RTimer::get_seconds()
{
    auto now = clk::now();
    auto elapsed = now - start_t;
    auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    return static_cast<double>(ms) / 1000;
}

void RTimer::update()
{
    auto now = clk::now();
    auto seconds = get_seconds();
    while (now >= next_timer) {
        if (timer_handler) {
            call_proc(ruby, timer_handler, seconds);
        }
        next_timer += 1ms * timer_interval;
    }
}

void RTimer::reg_class(mrb_state* ruby)
{
    rclass = mrb_define_class(ruby, "Timer", ruby->object_class);
    MRB_SET_INSTANCE_TT(rclass, MRB_TT_DATA);

    mrb_define_class_method(
        ruby, rclass, "default",
        [](mrb_state* mrb, mrb_value /*self*/) -> mrb_value {
            if (default_timer == nullptr) { default_timer = new RTimer(mrb); }
            return mrb::new_data_obj(mrb, default_timer);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, rclass, "seconds",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            return mrb::to_value(
                mrb::self_to<RTimer>(self)->get_seconds(), mrb);
        },
        MRB_ARGS_NONE());

    mrb_define_method(
        ruby, rclass, "on_timer",
        [](mrb_state* mrb, mrb_value self) -> mrb_value {
            auto* timer = mrb::self_to<RTimer>(self);
            mrb_int n = -1;
            mrb_value blk;
            mrb_get_args(mrb, "i&", &n, &blk);
            timer->timer_interval = n;
            if (!mrb_nil_p(blk)) {
                timer->timer_handler = mrb::RubyPtr{mrb, blk};
            }
            return mrb_nil_value();
        },
        MRB_ARGS_BLOCK() | MRB_ARGS_REQ(1));
}

void RTimer::reset()
{
    timer_handler.clear();
}

RTimer::RTimer(mrb_state* _ruby) : ruby{_ruby}
{
    start_t = clk::now();
    next_timer = start_t + 500ms;
}
