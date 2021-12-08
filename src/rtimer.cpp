#include "rtimer.hpp"
#include "error.hpp"

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
        if (timer_handler) { timer_handler(seconds); }
        next_timer += 1ms * timer_interval;
    }
}

void RTimer::reg_class(mrb_state* ruby)
{
    mrb::make_noinit_class<RTimer>(ruby, "Timer");
    mrb::set_deleter<RTimer>(ruby, [](mrb_state*, void*) {});
    default_timer = new RTimer(ruby);

    mrb::add_class_method<RTimer>(
        ruby, "default", [] { return RTimer::default_timer; });
    mrb::add_method<&RTimer::get_seconds>(ruby, "seconds");
    mrb::add_method<RTimer>(
        ruby, "on_timer", [](RTimer* self, int n, mrb::Block callback) {
            self->timer_interval = n;
            self->timer_handler = callback;
        });
}

void RTimer::reset()
{
    timer_handler.clear();
}

RTimer::RTimer(mrb_state*)
{
    start_t = clk::now();
    next_timer = start_t + 500ms;
}
