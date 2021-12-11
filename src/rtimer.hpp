#pragma once

#include "mrb/mrb_tools.hpp"

#include <chrono>

class RTimer
{
    mrb::Value timer_handler;

    std::chrono::steady_clock::time_point start_t;
    std::chrono::steady_clock::time_point next_timer;
    int timer_interval = 1000;

public:
    explicit RTimer(mrb_state* _ruby);

    double get_seconds();
    void update();
    void reset();

    static inline RTimer* default_timer = nullptr;

    static void reg_class(mrb_state* ruby);
};
