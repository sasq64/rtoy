
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
        varying vec2 out_uv;
        //uniform vec4 colors[256];
        void main() {
              const vec4 text_color = vec4(1,1,1,1);
              const vec2 console_size = vec2(120.0, 50.0);
              const vec2 texture_size = vec2(256.0, 256.0);
              const vec2 half_texel = 1.0 / texture_size;
              const vec2 char_size = vec2(8.0, 16.0);
              const vec2 vn = char_size / texture_size;

              vec4 ux = texture2D(uv_tex, out_uv);
              vec2 uvf = fract(out_uv * console_size);
              vec2 uv = ux + uvf * vn - half_texel; 
              gl_FragColor = texture2D(in_tex, uv).a * text_color;
        })gl"};

    std::pair<int, int> next_pos{0, 0};

    std::unordered_map<char32_t, uint32_t> char_uvs;
    gl_wrap::Texture font_texture;
    gl_wrap::Texture uv_texture;
    gl_wrap::Program program;
    std::vector<uint32_t> data;
    std::vector<uint32_t> uvdata;

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
        program = gl_wrap::Program({vertex_shader}, {fragment_shader});
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

    void render()
    {
        namespace gl = gl_wrap;
        float x = 1;
        float y = 1;

        float sx = 0;
        float sy = 0;
        for(auto& u : uvdata) {
            u = char_uvs['!' + rand() % 0x40];
        }
        uv_texture.update(uvdata.data());

        std::array<float, 8> uvs{sx, y, x, y, x, sy, sx, sy};
        glEnable(GL_BLEND);
        pix::set_colors(0xffffffff, 0);
        auto [w, h] = gl::getViewport();
        uv_texture.bind(1);
        font_texture.bind(0);
        program.setUniform("in_tex", 0);
        program.setUniform("uv_tex", 1);
        // program.setUniform("in_transform", transform);
        program.use();
        pix::draw_quad_uvs(uvs);
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

    PixConsole con{120, 50};

    con.text(2, 2, "ZweZxywXVwooPO");
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
