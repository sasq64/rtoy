#pragma once
#include "mrb/mrb_tools.hpp"

class RSpeech
{
    void* voice;
    mrb_state* ruby;
    static inline RSpeech* default_speech = nullptr;
public:

    explicit RSpeech(mrb_state*);
    void reset();
    static void reg_class(mrb_state* ruby);
};
