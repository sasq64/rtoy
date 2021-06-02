#pragma once

#include <array>
#include <mruby.h>
#include <mruby/data.h>
#include <pix/font.hpp>
#include <string>

struct RClass;
struct RImage;
struct mrb_state;

class RFont
{

public:
    FTFont font;
    static mrb_data_type dt;
    static inline RClass* rclass = nullptr;
    static void reg_class(mrb_state* ruby);

    RImage* render(std::string const& txt, int n);
    RFont(std::string const& name);
};

