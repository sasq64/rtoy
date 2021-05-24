#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <string_view>
#include <tuple>

namespace pix {

using Float2 = std::pair<float, float>;

void draw_quad_impl(float x, float y, float sx, float sy);
void draw_quad();

static constexpr size_t X = 0;
static constexpr size_t Y = 1;

inline void draw_quad(Float2 pos, Float2 size)
{
    auto [x, y] = pos;
    auto [w, h] = size;
    draw_quad_impl(x, y, w, h);
}

template <typename Point, typename Size>
inline void draw_quad(Point pos, Size size)
{
    auto [x, y] = pos;
    auto [w, h] = size;
    draw_quad_impl(x, y, w, h);
}

void draw_line_impl(float x0, float y0, float x1, float y1);

inline void draw_line(Float2 p0, Float2 p1)
{
    auto [x0, y0] = p0;
    auto [x1, y1] = p1;
    draw_line_impl(x0, y0, x1, y1);
}

template <typename Point, typename Size>
inline void draw_line(Point p0, Size p1)
{
    auto [x0, y0] = p0;
    auto [x1, y1] = p1;
    draw_line_impl(x0, y0, x1, y1);
}

void draw_circle_impl(float x, float y, float r);

inline void draw_circle(Float2 p, float r)
{
    auto [x, y] = p;
    draw_circle_impl(x, y, r);
}

template <typename Point>
inline void draw_circle(Point p, float r)
{
    auto [x, y] = p;
    draw_circle_impl(x, y, r);
}

void set_transform(std::array<float, 16> const& mat);
void set_colors(uint32_t fg, uint32_t bg);

struct Image
{
    Image() = default;
    unsigned width = 0;
    unsigned height = 0;
    std::shared_ptr<std::byte> sptr;
    std::byte* ptr = nullptr;
    unsigned format = 0;
};

Image load_png(std::string_view name);
void save_png(Image const& image, std::string_view name);

} // namespace pix
