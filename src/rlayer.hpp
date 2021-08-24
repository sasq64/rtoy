#pragma once

#include <array>
#include <cstdint>

struct RClass;
struct mrb_state;

enum BlendMode
{
    Blend,
    Add
};

struct RStyle
{
    std::array<float, 4> fg{};
    std::array<float, 4> bg{};
    float line_width = 1;
    BlendMode blend_mode = BlendMode::Blend;
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


public:
    virtual void reset();

    static inline RClass* rclass = nullptr;
    static void reg_class(mrb_state* ruby);

    RLayer(int w, int h) : width(w), height(h) {}

    RStyle style{{1,1,1,1}, {0,0,0,0}, 2.0F};
    virtual void render() {}
    virtual void update_tx();
};

