#pragma once

#include <mruby.h>
#include <mruby/data.h>
#include <mrb_tools.hpp>

#include <chrono>
#include <memory>

class RTimer
{
    mrb_state* ruby;
    //std::shared_ptr<void> timer_handler{};
    mrb::RubyPtr timer_handler;

    std::chrono::steady_clock::time_point start_t;
    std::chrono::steady_clock::time_point next_timer;
    int timer_interval = 1000;

public:
    explicit RTimer(mrb_state* _ruby);

    double get_seconds();
    void update();
    void reset();

    static inline RTimer* default_timer = nullptr;
    static inline RClass* rclass;
    static inline mrb_data_type dt{"Timer", [](mrb_state*, void* data) {}};

    static void reg_class(mrb_state* ruby);
};
