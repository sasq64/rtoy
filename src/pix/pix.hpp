#pragma once

#include <gl/gl.hpp>

#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string_view>
#include <tuple>

namespace gl_wrap {
struct Color;
} // namespace gl_wrap

namespace pix {

using Float2 = std::pair<double, double>;

void draw_quad_impl(double x, double y, double sx, double sy);
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

void draw_line_impl(double x0, double y0, double x1, double y1);
void draw_quad_filled(double x, double y, double sx, double sy);

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
    double x, double y, double sx, double sy, std::array<float, 8> const& uvs);

void draw_quad_uvs(std::array<float, 8> const& uvs);

void draw_circle_impl(double x, double y, double r);

inline void draw_circle(Float2 p, double r)
{
    auto [x, y] = p;
    draw_circle_impl(x, y, r);
}

template <typename Point>
inline void draw_circle(Point p, double r)
{
    auto [x, y] = p;
    draw_circle_impl(x, y, r);
}

void set_transform();
void set_transform(std::array<float, 16> const& mat);
void set_colors(gl_wrap::Color fg, gl_wrap::Color bg);

struct Image
{
    Image() = default;
    Image(int w, int h)
        : width(w),
          height(h),
          sptr{new std::byte[static_cast<size_t>(w) * h * 4]},
          ptr{sptr.get()},
          format{GL_RGBA}
    {}
    Image(int w, int h, std::byte* p, unsigned f = 0)
        : width(w), height(h), sptr{nullptr}, ptr{p}, format{f}
    {}

    void fill(uint32_t pixel) const
    {
        auto* pixel_ptr = reinterpret_cast<uint32_t*>(ptr);
        std::fill(
            pixel_ptr, pixel_ptr + static_cast<size_t>(width) * height, pixel);
    }
    int width = 0;
    int height = 0;
    std::shared_ptr<std::byte> sptr;
    std::byte* ptr = nullptr;
    unsigned format = 0;
};

Image load_png(std::string_view name);
Image load_jpg(std::filesystem::path const& name);

void save_png(Image const& image, std::string_view name);

} // namespace pix
