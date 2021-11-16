#pragma once
#include "mrb/mrb_tools.hpp"

class RSpeech
{
    void* voice;
    mrb_state* ruby;
    static inline RSpeech* default_speech = nullptr;
public:
    static inline RClass* rclass;
    static inline mrb_data_type dt{"Speech", [](mrb_state*, void* data) {}};

    explicit RSpeech(mrb_state*);
    void reset();
    static void reg_class(mrb_state* ruby);
};
