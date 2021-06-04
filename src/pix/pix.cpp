#include "pix.hpp"

#include <array>
#include <gl/buffer.hpp>
#include <gl/functions.hpp>
#include <gl/program_cache.hpp>
#include <gl/vec.hpp>
#include <lodepng.h>

namespace pix {
namespace gl = gl_wrap;

void set_transform(std::array<float, 16> const& mat)
{
    auto& textured = gl::ProgramCache::get_instance().textured;
    textured.setUniform("in_transform", mat);
    auto& plain = gl::ProgramCache::get_instance().non_textured;
    plain.setUniform("in_transform", mat);
}

void set_colors(uint32_t fg, uint32_t bg)
{
    auto& textured = gl::ProgramCache::get_instance().textured;
    textured.setUniform("in_color", gl::Color(fg));
    auto& plain = gl::ProgramCache::get_instance().non_textured;
    plain.setUniform("in_color", gl::Color(fg));
}

void draw_quad_invy()
{
    float x0 = -1;
    float y0 = -1;

    float x1 = 1;
    float y1 = 1;

    std::array vertexData{
        x0, y0, x1, y0, x1, y1, x0, y1, 0.F, 1.F, 1.F, 1.F, 1.F, 0.F, 0.F, 0.F};
    gl::ArrayBuffer<GL_STREAM_DRAW> vbo{vertexData};

    vbo.bind();

    auto& program = gl::ProgramCache::get_instance().textured;
    program.use();
    auto pos = program.getAttribute("in_pos");
    auto uv = program.getAttribute("in_uv");
    pos.enable();
    uv.enable();

    gl::vertexAttrib(pos, 2, gl::Type::Float, 0 * sizeof(GLfloat), 0);
    gl::vertexAttrib(uv, 2, gl::Type::Float, 0 * sizeof(GLfloat), 8 * 4);
    gl::drawArrays(gl::Primitive::TriangleFan, 0, 4);
    pos.disable();
    uv.disable();
}

void draw_quad()
{
    float x0 = -1;
    float y0 = -1;

    float x1 = 1;
    float y1 = 1;

    std::array vertexData{
        x0, y0, x1, y0, x1, y1, x0, y1, 0.F, 0.F, 1.F, 0.F, 1.F, 1.F, 0.F, 1.F};
    gl::ArrayBuffer<GL_STREAM_DRAW> vbo{vertexData};

    vbo.bind();

    auto& program = gl::ProgramCache::get_instance().textured;
    program.use();
    auto pos = program.getAttribute("in_pos");
    auto uv = program.getAttribute("in_uv");
    pos.enable();
    uv.enable();

    gl::vertexAttrib(pos, 2, gl::Type::Float, 0 * sizeof(GLfloat), 0);
    gl::vertexAttrib(uv, 2, gl::Type::Float, 0 * sizeof(GLfloat), 8 * 4);
    gl::drawArrays(gl::Primitive::TriangleFan, 0, 4);
    pos.disable();
    uv.disable();
}

void draw_quad_impl(float x, float y, float sx, float sy)
{
    auto [w, h] = gl::getViewport();

    auto x0 = x * 2.0F / w - 1.0F;
    auto y0 = y * -2.0F / h + 1.0F;

    x += sx;
    y += sy;

    auto x1 = x * 2.0F / w - 1.0F;
    auto y1 = y * -2.0F / h + 1.0F;

    std::array vertexData{
        x0, y0, x1, y0, x1, y1, x0, y1, 0.F, 0.F, 1.F, 0.F, 1.F, 1.F, 0.F, 1.F};
    gl::ArrayBuffer<GL_STREAM_DRAW> vbo{vertexData};

    vbo.bind();

    auto& program = gl::ProgramCache::get_instance().textured;
    program.use();
    auto pos = program.getAttribute("in_pos");
    auto uv = program.getAttribute("in_uv");
    // program.setUniform("in_color", gl::Color(0xffffffff));
    pos.enable();
    uv.enable();

    gl::vertexAttrib(pos, 2, gl::Type::Float, 0 * sizeof(GLfloat), 0);
    gl::vertexAttrib(uv, 2, gl::Type::Float, 0 * sizeof(GLfloat), 8 * 4);
    gl::drawArrays(gl::Primitive::TriangleFan, 0, 4);
    pos.disable();
    uv.disable();
}

Image load_png(std::string_view name)
{
    Image image;
    std::byte* out{};
    auto err = lodepng_decode32_file(reinterpret_cast<unsigned char**>(&out),
        &image.width, &image.height, std::string(name).c_str());
    if (err != 0) { return image; }
    image.sptr = std::shared_ptr<std::byte>(out, &free);
    image.ptr = out;
    image.format = GL_RGBA;
    return image;
}

void draw_line_impl(float x0, float y0, float x1, float y1)
{
    auto [w, h] = gl::getViewport();

    x0 = x0 * 2.0F / w - 1.0F;
    x1 = x1 * 2.0F / w - 1.0F;
    y0 = y0 * -2.0F / h + 1.0F;
    y1 = y1 * -2.0F / h + 1.0F;

    std::array vertexData{x0, y0, x1, y1};
    gl::ArrayBuffer<GL_STREAM_DRAW> vbo{vertexData};

    vbo.bind();
    // glBindBuffer(GL_

    auto& program = gl::ProgramCache::get_instance().non_textured;
    program.use();
    auto pos = program.getAttribute("in_pos");
    pos.enable();
    // program.setUniform("in_color", gl::Color(0xffff00ff));
    program.setUniform("in_transform",
        std::array{1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F,
            1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F});

    auto vp = gl_wrap::getViewport();

    gl::vertexAttrib(0, gl::Size<2>{}, gl::Type::Float, 0 * sizeof(GLfloat), 0);
    gl::drawArrays(gl::Primitive::Lines, 0, 2);
    pos.disable();
}

void draw_circle_impl(float x, float y, float radius)
{
    auto [w, h] = gl::getViewport();
    int steps = static_cast<int>(M_PI / asinf(sqrtf(1 / radius)));

    steps = 64;

    std::vector<float> vertexData;
    vertexData.push_back(x * 2.0F / w - 1.0F);
    vertexData.push_back(y * -2.0F / h + 1.0F);
    for (int i = 0; i <= steps; i++) {
        auto px = cosf(M_PI * 2.0 * i / steps) * radius + x;
        auto py = sinf(M_PI * 2.0 * i / steps) * radius + y;
        px = px * 2.0F / w - 1.0F;
        py = py * -2.0F / h + 1.0F;
        vertexData.push_back(px);
        vertexData.push_back(py);
    }
    gl::ArrayBuffer<GL_STREAM_DRAW> vbo{vertexData};

    vbo.bind();

    auto& program = gl::ProgramCache::get_instance().non_textured;
    program.use();
    auto pos = program.getAttribute("in_pos");
    pos.enable();
    // program.setUniform("in_color", gl::Color(0xffff00ff));
    program.setUniform("in_transform",
        std::array{1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F,
            1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F});

    gl::vertexAttrib(0, gl::Size<2>{}, gl::Type::Float, 0 * sizeof(GLfloat), 0);
    gl::drawArrays(gl::Primitive::TriangleFan, 0, steps + 2);
    pos.disable();
}

void save_png(Image const& image, std::string_view name)
{

    lodepng_encode_file(std::string(name).c_str(),
        (unsigned char const*)image.ptr, image.width, image.height, LCT_RGBA,
        8);

    return;

    unsigned error{};
    LodePNGState state;
    lodepng_state_init(&state);

    std::vector<uint32_t> colors(256);
    for (int i = 0; i < 256; i++) {
        colors[i] = i | (i << 8) | (i << 16) | 0xff000000;
    }
    // Input format
    state.info_raw.colortype = LCT_GREY;
    state.info_raw.bitdepth = 8;
    // state.info_raw.palette = (unsigned char*)colors.data();
    // state.info_raw.palettesize = colors.size();

    // Output format
    state.info_png.color.colortype = LCT_GREY; // PALETTE;
    state.info_png.color.bitdepth = 8;
    // state.info_png.color.palettesize = colors.size();
    // state.info_png.color.palette =
    //    reinterpret_cast<unsigned char*>(colors.data());

    state.encoder.auto_convert = 0;
    // state.encoder.force_palette = 1;

    unsigned char* buffer{};
    size_t buffersize{};
    lodepng_encode(&buffer, &buffersize,
        reinterpret_cast<unsigned char*>(image.ptr), image.width, image.height,
        &state);
    error = state.error;

    if (error == 0) {
        error =
            lodepng_save_file(buffer, buffersize, std::string(name).c_str());
        fmt::print("Saved");
    } else {
        fmt::print("Error {}\n", error);
    }
    free(buffer);

    // LodePNGColorType lt = LCT_PALETTE;
    // lodepng_encode_file(name, image.ptr, image.width, image.height, lt, 8);
}

} // namespace pix
