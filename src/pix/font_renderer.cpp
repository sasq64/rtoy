#include "font_renderer.hpp"
#include <coreutils/utf8.h>

void FontRenderer::add_text(
    std::pair<float, float> xy, TextAttrs const& attrs, std::string_view text)
{
    add_text(xy, attrs, utils::utf8_decode(text));
}

void FontRenderer::add_text(std::pair<float, float> xy, TextAttrs const& attrs,
    std::u32string_view text32)
{
    // Generate UVs for this text
    std::vector<UV> uv_data(text32.size() * 2);
    auto target = uv_data.begin();
    for (char32_t c : text32) {
        if (c == 1) { continue; }
        auto it = uv_map.find(c);
        auto l = it->second;
        *target++ = l.uv;
        if(l.tindex > 0) {
            fmt::print("Adding {},{} {},{}\n", l.uv[0].x(), l.uv[0].y(), l.uv[2].x(), l.uv[2].y());
        }
    }
    size_t size = target - uv_data.begin();
    // Upload UVs
    text_buffer.update(uv_data.data(), uv_buffer_pos, size * 8 * 4);
    objects.push_back({xy, uv_buffer_pos, size, attrs});
    uv_buffer_pos += size * 8 * 4;
}

void FontRenderer::render()
{
    namespace gl = gl_wrap;

    glDisable(GL_BLEND);

    program.use();
    text_buffer.bind();

    auto vp = gl_wrap::getViewport();
    auto ss = std::make_pair(2.0F / vp.first, 2.0F / vp.second);
    program.setUniform("screen_scale", ss);

    auto pos = program.getAttribute("in_pos");
    auto uv = program.getAttribute("in_uv");

    pos.enable();
    uv.enable();

    gl::vertexAttrib(pos, 2, gl::Type::Float, 0, 0);

    index_buffer.bind();
    for (auto const& obj : objects) {
        program.setUniform("fg_color", gl::Color(obj.attrs.fg));
        program.setUniform("bg_color", gl::Color(obj.attrs.bg));
        program.setUniform("scale", obj.attrs.scale);

        auto xy = obj.xy;
        xy.first -= (vp.first / 2.0F);
        xy.second -= (vp.second / 2.0F);
        program.setUniform("xypos", xy);

        gl::vertexAttrib(uv, 2, gl::Type::Float, 0, obj.buffer_offset);

        gl::drawElements(
            gl::Primitive::Triangles, 6 * obj.size, gl::Type::UnsignedShort, 0);
    }
    objects.clear();
    uv_buffer_pos = max_text_length * 8 * 4;
    glEnable(GL_BLEND);
}

void FontRenderer::add_char_location(char32_t c, int x, int y, int w, int h)
{
    auto fx = static_cast<float>(x);
    auto fy = static_cast<float>(y);
    auto fw = static_cast<float>(w);
    auto fh = static_cast<float>(h);
    UV uv = {vec2{fx, fy}, {fx + fw, fy}, {fx, fy + fh}, {fx + fw, fy + fh}};
    uv_map[c] = { uv, 0 };
}

void FontRenderer::add_char_location(char32_t c, UV const& uv, int ti)
{
    uv_map[c] = { uv, ti };
}

FontRenderer::FontRenderer(int cw, int ch)
    : char_width(cw),
      char_height(ch),
      text_buffer(1000000),
      index_buffer(max_text_length * 6 * 2)
{
    program = gl_wrap::Program({vertex_shader}, {pixel_shader});

    program.use();
    program.setUniform("scale", 1.0F);
    std::vector<vec2> points;
    vec2 xy{0, 0};
    for (unsigned i = 0; i < max_text_length; i++) {
        points.push_back(xy);
        points.push_back(xy + vec2{char_width, 0});
        points.push_back(xy + vec2{char_width, char_height});
        points.push_back(xy + vec2{0, char_height});
        xy += {char_width, 0};
    }
    text_buffer.update(points, 0);
    uv_buffer_pos = max_text_length * 8 * 4;

    std::vector<uint16_t> indexes;
    for (unsigned i = 0; i < max_text_length; i++) {
        auto n = i * 4;
        indexes.push_back(n);
        indexes.push_back(n + 1);
        indexes.push_back(n + 2);
        indexes.push_back(n + 0);
        indexes.push_back(n + 2);
        indexes.push_back(n + 3);
    }
    index_buffer.set(indexes);
}
