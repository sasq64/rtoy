#pragma once

#include <array>
#include <cstdint>

struct RClass;
struct mrb_state;

struct RStyle
{
    uint32_t fg;
    uint32_t bg;
    float line_width;
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

    void update_tx();

public:
    virtual void reset();

    static inline RClass* rclass = nullptr;
    static void reg_class(mrb_state* ruby);

    RLayer(int w, int h) : width(w), height(h) {}

    RStyle style{0xffffffff, 0x00000000, 2.0F};
    virtual void render() {}
};

