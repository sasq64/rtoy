#include "pix.hpp"

#include <array>
#include <fstream>
#include <istream>
#include <filesystem>

#include <gl/buffer.hpp>
#include <gl/functions.hpp>
#include <gl/program_cache.hpp>
#include <gl/vec.hpp>
#include <jpeg_decoder.h>
#include <lodepng.h>

namespace fs = std::filesystem;

namespace pix {
namespace gl = gl_wrap;

void set_transform()
{
    static const std::array<float, 16> mat{
        1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    auto& textured = gl::ProgramCache::get_instance().textured;
    textured.setUniform("in_transform", mat);
    auto& plain = gl::ProgramCache::get_instance().non_textured;
    plain.setUniform("in_transform", mat);
}

void set_transform(std::array<float, 16> const& mat)
{
    auto& textured = gl::ProgramCache::get_instance().textured;
    textured.setUniform("in_transform", mat);
    auto& plain = gl::ProgramCache::get_instance().non_textured;
    plain.setUniform("in_transform", mat);
}

void set_colors(gl::Color fg, gl::Color bg)
{
    auto& textured = gl::ProgramCache::get_instance().textured;
    textured.setUniform("in_color", fg);
    auto& plain = gl::ProgramCache::get_instance().non_textured;
    plain.setUniform("in_color", fg);
}

void draw_with_uvs()
{
    // gl::ProgramCache::get_instance().textured.use();
    auto& program = gl::Program::current();
    // auto& program = gl::ProgramCache::get_instance().textured;
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
    draw_with_uvs();
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
    draw_with_uvs();
}

void draw_quad_uvs(std::array<float, 8> const& uvs)
{
    std::array vertexData{-1.F, -1.F, 1.F, -1.F, 1.F, 1.F, -1.F, 1.F, 0.F, 0.F,
        1.F, 0.F, 1.F, 1.F, 0.F, 1.F};
    std::copy(uvs.begin(), uvs.end(), vertexData.begin() + 8);
    gl::ArrayBuffer<GL_STREAM_DRAW> vbo{vertexData};

    vbo.bind();
    draw_with_uvs();
}

void draw_quad_uvs(
    double x, double y, double sx, double sy, std::array<float, 8> const& uvs)
{
    auto [w, h] = gl::getViewport<float>();

    float x0 = static_cast<float>(x) * 2.0F / w - 1.0F;
    float y0 = static_cast<float>(y) * -2.0F / h + 1.0F;

    x += sx;
    y += sy;

    float x1 = static_cast<float>(x) * 2.0F / w - 1.0F;
    float y1 = static_cast<float>(y) * -2.0F / h + 1.0F;

    std::array vertexData{
        x0, y0, x1, y0, x1, y1, x0, y1, 0.F, 0.F, 1.F, 0.F, 1.F, 1.F, 0.F, 1.F};

    std::copy(uvs.begin(), uvs.end(), vertexData.begin() + 8);

    gl::ArrayBuffer<GL_STREAM_DRAW> vbo{vertexData};

    vbo.bind();
    draw_with_uvs();
}

void draw_quad_impl(double x, double y, double sx, double sy)
{
    auto [w, h] = gl::getViewport<float>();

    float x0 = static_cast<float>(x) * 2.0F / w - 1.0F;
    float y0 = static_cast<float>(y) * -2.0F / h + 1.0F;

    x += sx;
    y += sy;

    float x1 = static_cast<float>(x) * 2.0F / w - 1.0F;
    float y1 = static_cast<float>(y) * -2.0F / h + 1.0F;

    std::array vertexData{
        x0, y0, x1, y0, x1, y1, x0, y1, 0.F, 0.F, 1.F, 0.F, 1.F, 1.F, 0.F, 1.F};
    gl::ArrayBuffer<GL_STREAM_DRAW> vbo{vertexData};

    vbo.bind();
    draw_with_uvs();
}

void draw_quad_filled(double x, double y, double sx, double sy)
{
    auto [w, h] = gl::getViewport<float>();

    float x0 = static_cast<float>(x) * 2.0F / w - 1.0F;
    float y0 = static_cast<float>(y) * -2.0F / h + 1.0F;

    x += sx;
    y += sy;

    float x1 = static_cast<float>(x) * 2.0F / w - 1.0F;
    float y1 = static_cast<float>(y) * -2.0F / h + 1.0F;

    std::array vertexData{x0, y0, x1, y0, x1, y1, x0, y1};
    gl::ArrayBuffer<GL_STREAM_DRAW> vbo{vertexData};

    vbo.bind();

    auto& program = gl::ProgramCache::get_instance().non_textured;
    program.use();
    auto pos = program.getAttribute("in_pos");
    pos.enable();

    gl::vertexAttrib(pos, 2, gl::Type::Float, 0 * sizeof(GLfloat), 0);
    gl::drawArrays(gl::Primitive::TriangleFan, 0, 4);
    pos.disable();
}

template <typename Result = std::string, typename Stream>
static Result read_all(Stream& in)
{
    Result contents;
    auto here = in.tellg();
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg() - here);
    in.seekg(here, std::ios::beg);
    in.read((char*)contents.data(), contents.size());
    in.close();
    return contents;
}

Image load_jpg(fs::path const& name)
{
    std::ifstream jpg_file;
    jpg_file.open(name);
    auto data = read_all<std::vector<uint8_t>>(jpg_file);
    jpeg::Decoder decoder{data.data(), data.size()};

    Image image{decoder.GetWidth(), decoder.GetHeight()};
    auto* rgb = decoder.GetImage();
    uint32_t* ptr = (uint32_t*)image.ptr;
    for (int i=0; i< image.width*image.height; i++) {
        auto j = i*3;
        ptr[i] = (rgb[j+2] << 16) | (rgb[j+1] << 8) | (rgb[j]) | (0xff000000);
    }
    //memcpy(image.ptr, decoder.GetImage(), image.width * image.height * 4);
    return image;
}

Image load_png(std::string_view name)
{
    fmt::print("Loading {}\n", name);
    Image image;
    std::byte* out{};
    unsigned w = 0;
    unsigned h = 0;
    auto err = lodepng_decode32_file(reinterpret_cast<unsigned char**>(&out),
        &w, &h, std::string(name).c_str());
    if (err != 0) { return image; }
    image.width = static_cast<int>(w);
    image.height = static_cast<int>(h);
    image.sptr = std::shared_ptr<std::byte>(out, &free);
    image.ptr = out;
    image.format = GL_RGBA;
    return image;
}

void draw_line_impl(double x0, double y0, double x1, double y1)
{
    auto [w, h] = gl::getViewport<float>();

    fmt::print("{} {}\n", w, h);
    fmt::print("{} {} {} {}\n", x0, y0, x1, y1);

    std::array vertexData{static_cast<float>(x0) * 2.0F / w - 1.0F,
        static_cast<float>(y0) * -2.0F / h + 1.0F,
        static_cast<float>(x1) * 2.0F / w - 1.0F,
        static_cast<float>(y1) * -2.0F / h + 1.0F};

    gl::ArrayBuffer<GL_STREAM_DRAW> vbo{vertexData};

    vbo.bind();

    auto& program = gl::ProgramCache::get_instance().non_textured;
    program.use();
    auto pos = program.getAttribute("in_pos");
    pos.enable();
    program.setUniform("in_transform",
        std::array{1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F,
            1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F});

    gl::vertexAttrib(0, gl::Size<2>{}, gl::Type::Float, 0 * sizeof(GLfloat), 0);
    gl::drawArrays(gl::Primitive::Lines, 0, 2);
    pos.disable();
}

void draw_circle_impl(double x, double y, double radius)
{
    auto [w, h] = gl::getViewport<float>();
    int steps = static_cast<int>(M_PI / asin(sqrt(1.0 / radius)));

    steps = 64;

    std::vector<float> vertexData;
    vertexData.push_back(static_cast<float>(x) * 2.0F / w - 1.0F);
    vertexData.push_back(static_cast<float>(y) * -2.0F / h + 1.0F);
    for (int i = 0; i <= steps; i++) {
        auto px =
            static_cast<float>((cos(M_PI * 2.0 * i / steps)) * radius + x);
        auto py =
            static_cast<float>((sin(M_PI * 2.0 * i / steps)) * radius + y);
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
    auto* ptr = reinterpret_cast<unsigned char*>(image.ptr);
    /* for (int i = 0; i < image.width * image.height * 4; i++) { */
    /*     if ((i & 3) == 3) { ptr[i] = 0xff; } */
    /* } */
    lodepng_encode_file(
        std::string(name).c_str(), ptr, image.width, image.height, LCT_RGBA, 8);
}

#if 0
void save_png_gray(Image const& image, std::string_view name)
{

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
#endif

} // namespace pix
