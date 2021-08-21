
#include "src/system.hpp"
#include <gl/color.hpp>
#include <gl/functions.hpp>
#include <gl/gl.hpp>
#include <gl/program.hpp>
#include <gl/texture.hpp>
#include <pix/font.hpp>

#include <pix/pix.hpp>

#include <variant>

namespace gl = gl_wrap;

int texture_width = 256;
int texture_height = 256;
int char_width = 8;
int char_height = 16;

    std::string vertex_shader{R"gl( 
    #ifdef GL_ES
        precision mediump float;
    #endif
        attribute vec2 in_pos;
        //uniform mat4 in_transform;
        attribute vec2 in_uv;
        varying vec2 out_uv;
        void main() {
            vec4 v = vec4(in_pos, 0, 1);
            gl_Position = vec4( v.x, v.y, 0, 1 );
            out_uv = in_uv;
        })gl"};

    std::string fragment_shader{R"gl(
    #ifdef GL_ES
        precision mediump float;
    #endif
        uniform vec4 in_color;
        uniform sampler2D in_tex;
        uniform sampler2D uv_tex;
        varying vec2 out_uv;
        //uniform vec4 colors[256];
        void main() {
              const vec4 text_color = vec4(1,1,1,1);
              const vec2 console_size = vec2(32.0, 32.0);
              const vec2 texture_size = vec2(256.0, 256.0);
              const vec2 half_texel = 0.75 / texture_size;
              const vec2 char_size = vec2(8.0, 16.0);
              const vec2 vn = char_size / texture_size;

              vec4 ux = texture2D(uv_tex, out_uv);
              vec2 uvf = fract(out_uv * console_size);
              vec2 uv = ux + uvf * vn - half_texel; 
              gl_FragColor = texture2D(in_tex, uv).a * text_color;
        })gl"};

#if 0
static inline char const* pixel_shader =
    R"gl(
    #ifdef GL_ES
        precision mediump float;
    #endif
        uniform sampler2D in_tex;
        varying vec2 out_uv;
        uniform vec4 fg_color;
        uniform vec4 bg_color;
        void main() {
            vec4 col = texture2D(in_tex, vec2(out_uv.x, - out_uv.y));
            float a = col.a;
            col.a = mix(a, 1.0, bg_color.a);
            col.rgb = mix(fg_color.rgb * col.rgb, bg_color.rgb,
                          (1.0 - a) * bg_color.a);
            gl_FragColor = col;
        }
    )gl";
#endif

std::pair<int, int> next_pos{0, 0};

std::unordered_map<char32_t, uint32_t> char_uvs;

void add_char(char32_t c, uint32_t* data, FTFont& font)
{
    if (c == 1) { return; }

    // First render character into texture
    auto* ptr = &data[next_pos.first + next_pos.second * texture_width];
    int x = next_pos.first;
    int y = next_pos.second;
    auto cw = font.render_char(c, ptr, 0xffffff00, texture_width);
    fmt::print("{}\n", cw);

    // then check if this char is wide and flag it
    if (cw < char_width) { cw = char_width; }

    char_uvs[c] = (x) | (y << 8);

    next_pos.first += char_width;
    if (next_pos.first >= (texture_width - char_width)) {
        next_pos.first = 0;
        next_pos.second += char_height;
    }
}

int main()
{
#ifdef RASPBERRY_PI
    auto system = create_pi_system();
#else
    auto system = create_sdl_system();
#endif

    Settings settings;

    auto window = system->init_screen(settings);

    system->init_input(settings);
    gl::Color bg{0xff0000ff};


    FTFont font{"data/unscii-16.ttf"};
    font.set_pixel_size(16);
    std::vector<uint32_t> data(256 * 256);
    std::fill(data.begin(), data.end(), 0);


    for (char32_t c = 0x20; c <= 0x7f; c++) {
        add_char(c, data.data(), font);
    }
    fmt::print("{:x}\n", char_uvs['C']);
    gl::Texture font_texture(256, 256, data);

    std::vector<uint32_t> uvdata(32 * 32);
    for (int i = 0; i < 32 * 32; i++) {
        uvdata[i] = char_uvs['Z' - (i % 26)];
    }
    uvdata[0] = char_uvs['a'];
    uvdata[31] = char_uvs['z'];
    uvdata[31*32] = char_uvs['1'];
    uvdata[32*32-1] = char_uvs['2'];
    gl::Texture uv_texture{32, 32, uvdata};
    auto vao = gl::genVertexArrays<1>();
    glBindVertexArray(vao[0]);

    auto program = gl_wrap::Program({vertex_shader}, {fragment_shader});

    float x = 0.25;
    float y = 0.25;

    float sx = 0;
    float sy = 0;

    uv_texture.bind(1);
    font_texture.bind(0);
    while (true) {

    std::array<float, 8> uvs {
        sx,y, x, y, x, sy, sx, sy
    };
        //x *= 1.01;
        //y *= 1.01;
        sx += 0.0001;
        x += 0.0001;
        auto event = system->poll_events();
        if (std::holds_alternative<QuitEvent>(event)) { break; }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        gl::clearColor(bg);
        glClear(GL_COLOR_BUFFER_BIT);
        //auto& program = gl::ProgramCache::get_instance().textured;
        //program.use();
        glEnable(GL_BLEND);
        pix::set_colors(0xffffffff, 0);
        auto [w, h] = gl::getViewport();
        program.setUniform("in_tex", 0);
        program.setUniform("uv_tex", 1);
    //program.setUniform("in_transform", transform);
        program.use();
        pix::draw_quad_uvs(uvs);
        //pix::draw_quad();

        window->swap();
    }
}
