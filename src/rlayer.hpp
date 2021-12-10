#pragma once

#include <array>
#include <cstdint>

#include "mrb/mrb_tools.hpp"

enum BlendMode
{
    Blend,
    Add
};

struct RStyle
{
    static constexpr const char* class_name() { return "Style"; }
    std::array<float, 4> fg{1, 1, 1, 1};
    std::array<float, 4> bg{0, 0, 0, 0};
    float line_width = 2.0F;
    BlendMode blend_mode = BlendMode::Blend;

    static void reg_class(mrb_state* ruby);
    static inline mrb_state* ruby = nullptr;
};

class RLayer
{
protected:
    std::array<float, 16> transform{
        1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
public:

    static constexpr const char* class_name() { return "Layer"; }

    std::array<float, 2> trans = {0.0F, 0.0F};
    std::array<float, 2> scale = {1.0F, 1.0F};
    float rot = 0.0;

    std::array<int, 4> scissor { 0, 0, 0, 0};

protected:
    int width = -1;
    int height = -1;

    bool enabled = true;

    BlendMode blend_mode = BlendMode::Blend;

    static inline mrb::Symbol blend_sym;
    static inline mrb::Symbol add_sym;

    mrb::Value stylep;
    RStyle& current_style;

public:
    // Reset all state of the layer to default.
    virtual void reset();
    // Clear the layer
    virtual void clear() {};

    static void reg_class(mrb_state* ruby);

    RLayer(int w, int h)
        : width(w),
          height(h),
          stylep{RStyle::ruby, new RStyle()},
          current_style{*stylep.as<RStyle*>()}
    {}

    virtual ~RLayer() = default;

    virtual void render(RLayer const* parent) {}
    virtual void update_tx(RLayer const* parent);

    virtual void enable(bool en = true) { enabled = en; }
};

