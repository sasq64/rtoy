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
        auto uv = it->second;

        // Width of character quad
        auto dx = (uv[1].x() - uv[0].x());
        if (dx > 8000) {
            // Handle double wide char by splitting it up in
            // two normal sized quads.
            // Move X coordinate down half a tile
            dx /= 2.0;
            uv[1].x() = uv[3].x() = (uv[0].x() + dx);
            *target++ = uv;
            // Then add half a tile to all X:es
            for (auto& t : uv) {
                t += {dx, 0};
            }
            *target++ = uv;
        } else {
            *target++ = uv;
        }
    }
    auto size = target - uv_data.begin();
    // Upload UVs
    text_buffer.update(uv_data.data(), uv_buffer_pos, size * 8 * 4);
    objects.push_back({xy, uv_buffer_pos, text32.size(), attrs});
    uv_buffer_pos += size * 8 * 4;
}

void FontRenderer::render()
{
    namespace gl = gl_wrap;

    glDisable(GL_BLEND);

    program.use();
    // texture.bind();
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
    uv_map[c] = uv;
}

FontRenderer::FontRenderer(int w, int h)
    : text_buffer(1000000), index_buffer(max_text_length * 6 * 2)
{
    program = gl_wrap::Program({vertex_shader}, {pixel_shader});

    program.use();
    auto ts = std::make_pair(
        1.0F / static_cast<float>(w), 1.0F / static_cast<float>(h));
    program.setUniform("tex_scale", ts);
    program.setUniform("scale", 1.0F);
    std::vector<vec2> points;
    vec2 xy{0, 0};
    for (unsigned i = 0; i < max_text_length; i++) {
        points.push_back(xy + vec2{0, char_height});
        points.push_back(xy + vec2{char_width, char_height});
        points.push_back(xy);
        points.push_back(xy + vec2{char_width, 0});
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
        indexes.push_back(n + 1);
        indexes.push_back(n + 3);
        indexes.push_back(n + 2);
    }
    index_buffer.set(indexes);
}
