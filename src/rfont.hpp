#pragma once

#include <array>
#include <pix/font.hpp>
#include <string>

class RImage;
struct mrb_state;

class RFont
{
public:
    FTFont font;
    static void reg_class(mrb_state* ruby);

    RImage* render(std::string const& txt, uint32_t color, int n);
    explicit RFont(std::string const& name);
};

