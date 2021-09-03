#pragma once

#include <array>
#include <cstdint>

#include "mrb_tools.hpp"

struct RClass;
struct mrb_state;

enum BlendMode
{
    Blend,
    Add
};

struct RStyle
{
    std::array<float, 4> fg{1, 1, 1, 1};
    std::array<float, 4> bg{0, 0, 0, 0};
    float line_width = 2.0F;
    BlendMode blend_mode = BlendMode::Blend;

    static inline RClass* rclass = nullptr;
    static mrb_data_type dt;
    static void reg_class(mrb_state* ruby);

    static inline mrb_state* ruby = nullptr;
};

class RLayer
{
protected:
    std::array<float, 16> transform{
        1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    std::array<float, 2> trans = {0.0F, 0.0F};
    std::array<float, 2> scale = {1.0F, 1.0F};
    float rot = 0.0;

    int width = -1;
    int height = -1;

    bool enabled = true;

    mrb::RubyPtr stylep;
    RStyle& current_style;

public:
    virtual void reset();

    static inline RClass* rclass = nullptr;
    static void reg_class(mrb_state* ruby);

    RLayer(int w, int h)
        : width(w),
          height(h),
          stylep{mrb::RubyPtr{RStyle::ruby,
              mrb_obj_new(RStyle::ruby, RStyle::rclass, 0, nullptr)}},
          current_style{*stylep.as<RStyle>()}
    {}

    virtual void render() {}
    virtual void update_tx();

    virtual void enable(bool en = true) { enabled = en; }
};

