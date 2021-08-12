#pragma once

#include <gl/gl.hpp>

#include <array>
#include <cstddef>
#include <memory>
#include <string_view>
#include <tuple>

namespace gl_wrap {
struct Color;
} // namespace gl_wrap

namespace pix {

using Float2 = std::pair<float, float>;

void draw_quad_impl(float x, float y, float sx, float sy);
void draw_quad();
void draw_quad_invy();

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
void draw_quad_filled(float x, float y, float sx, float sy);

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

void draw_quad_uvs(
    float x0, float y0, float w, float h, std::array<float, 8> const& uvs);
void draw_quad_uvs(std::array<float, 8> const& uvs);

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
void set_colors(gl_wrap::Color fg, gl_wrap::Color bg);

struct Image
{
    Image() = default;
    Image(unsigned w, unsigned h)
        : width(w),
          height(h),
          sptr{new std::byte[static_cast<size_t>(w) * h * 4]},
          ptr{sptr.get()},
          format{GL_RGBA}
    {}
    Image(unsigned w, unsigned h, std::byte* p, unsigned f = 0)
        : width(w), height(h), sptr{nullptr}, ptr{p}, format{f}
    {}

    void fill(uint32_t pixel) const
    {
        auto* pixel_ptr = reinterpret_cast<uint32_t*>(ptr);
        std::fill(pixel_ptr, pixel_ptr + width * height, pixel);
    }
    unsigned width = 0;
    unsigned height = 0;
    std::shared_ptr<std::byte> sptr;
    std::byte* ptr = nullptr;
    unsigned format = 0;
};

Image load_png(std::string_view name);
void save_png(Image const& image, std::string_view name);

} // namespace pix
