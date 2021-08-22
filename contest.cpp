
#include "src/system.hpp"
#include <gl/color.hpp>
#include <gl/functions.hpp>
#include <gl/gl.hpp>
#include <gl/program.hpp>
#include <gl/texture.hpp>
#include <pix/font.hpp>

#include <pix/pix.hpp>

#include <variant>

class PixConsole
{

    int texture_width = 256;
    int texture_height = 256;
    int char_width = 8;
    int char_height = 16;

    FTFont font;

    std::string vertex_shader{R"gl( 
    #ifdef GL_ES
        precision mediump float;
    #endif
        attribute vec2 in_pos;
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
        uniform vec2 console_size;
        uniform vec2 char_size;
        uniform vec2 texture_size;
        varying vec2 out_uv;
        uniform vec4 colors[8];
        void main() {
              //vec2 half_texel = 0.5 / texture_size;
              vec2 vn = char_size / texture_size;
              vec4 up = texture2D(uv_tex, out_uv);
              vec4 fg_color = colors[int(up.z * 256.0)];
              vec4 bg_color = colors[int(up.w * 256.0)];
              vec2 ux = floor(up.xy * 256.0) / 256.0;
              //ux = ux + half_texel;
              vec2 uvf = fract(out_uv * console_size);
              vec2 uv = ux + uvf * vn;// - half_texel; 
              vec4 col = texture2D(in_tex, uv);
              float a = col.a;
              col.a = mix(a, 1.0, bg_color.a);
              col.rgb = mix(fg_color.rgb * col.rgb, bg_color.rgb,
                            (1.0 - a) * bg_color.a);
              gl_FragColor = col;
        })gl"};

    std::pair<int, int> next_pos{0, 0};

    std::unordered_map<char32_t, uint32_t> char_uvs;
    gl_wrap::Texture font_texture;
    gl_wrap::Texture uv_texture;
    gl_wrap::Program program;
    std::vector<uint32_t> data;
    std::vector<uint32_t> uvdata;
    std::array<float, 8> uvs;

    unsigned width;
    unsigned height;

public:
    PixConsole(unsigned w, unsigned h)
        : font{"data/unscii-16.ttf"}, width(w), height(h)
    {
        font.set_pixel_size(16);
        data.resize(256 * 256);
        std::fill(data.begin(), data.end(), 0);

        for (char32_t c = 0x20; c <= 0x7f; c++) {
            add_char(c);
        }
        fmt::print("{:x}\n", char_uvs['C']);
        font_texture = gl_wrap::Texture{256, 256, data};

        uvdata.resize(w * h);
        std::fill(uvdata.begin(), uvdata.end(), 0);
        uv_texture = gl_wrap::Texture{w, h, uvdata};

        uv_texture.bind(1);
        font_texture.bind(0);
        float x = 1;
        float y = 1;

        float sx = 0;
        float sy = 0;
        uvs = {sx, y, x, y, x, sy, sx, sy};

        program = gl_wrap::Program({vertex_shader}, {fragment_shader});

        std::vector<gl_wrap::Color> colors;
        colors.emplace_back(0xffffffff);
        colors.emplace_back(0xff0000ff);
        colors.emplace_back(0x0000ffff);
        program.setUniform("colors", colors);

        program.setUniform("console_size", std::pair<float, float>(w, h));
        program.setUniform("texture_size", std::pair<float, float>(256, 256));
        program.setUniform("char_size", std::pair<float, float>(8, 16));
        for(auto& u : uvdata) {
            uint32_t x = char_uvs['!' + rand() % 0x40];
            x |= 0x01000000;
            u = x;
        }
        uv_texture.update(uvdata.data());
    }

    void text(int x, int y, std::string const& t)
    {
        auto text32 = utils::utf8_decode(t);
        for (auto c : text32) {
            if (c == 10) {
                x = 0;
                y++;
                continue;
            }

            auto& t = uvdata[x + width * y];
            t = char_uvs[c];
            t |= 0x02000000;
            x++;
            if (x >= width) {
                x = 0;
                y++;
            }
        }
        uv_texture.update(uvdata.data());
    }

    void add_char(char32_t c)
    {
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

    float s = 1.0;

    void render()
    {

        s *= 1.001;

        glEnable(GL_BLEND);
        pix::set_colors(0xffffffff, 0);
        //auto [w, h] = gl::getViewport();
        uv_texture.bind(1);
        font_texture.bind(0);
        program.setUniform("in_tex", 0);
        program.setUniform("uv_tex", 1);
        // program.setUniform("in_transform", transform);
        program.use();
        float x = 0;
        float y = 0;
        float w = s * width * 8.0F;
        float h = s * height * 16.0F;
        pix::draw_quad_impl(x, y, w, h);
    }
};

int main()
{
    namespace gl = gl_wrap;
#ifdef RASPBERRY_PI
    auto system = create_pi_system();
#else
    auto system = create_sdl_system();
#endif

    Settings settings;

    auto window = system->init_screen(settings);

    system->init_input(settings);
    gl::Color bg{0xffff00ff};

    PixConsole con{256, 256};

    con.text(2, 2, "ZweZxywXVwooPO");
    con.text(2, 30, "Hello citizens of the earth! This console is fast!");
    while (true) {

        auto event = system->poll_events();
        if (std::holds_alternative<QuitEvent>(event)) { break; }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        gl::clearColor(bg);
        glClear(GL_COLOR_BUFFER_BIT);
        // auto& program = gl::ProgramCache::get_instance().textured;
        // program.use();
        con.render();

        window->swap();
    }
}
