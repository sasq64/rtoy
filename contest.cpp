
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

    unsigned texture_width = 512;
    unsigned texture_height = 512;
    unsigned char_width = 13;
    unsigned char_height = 24;

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
        uniform sampler2D in_tex;
        uniform sampler2D uv_tex;
        uniform sampler2D col_tex;

        uniform vec2 console_size;
        uniform vec2 uv_scale;
        varying vec2 out_uv;

        void main() {
              vec4 up = texture2D(uv_tex, out_uv);
              vec4 color = texture2D(col_tex, out_uv);
              vec4 fg_color = vec4(up.wz, color.a, 1.0);
              vec4 bg_color = vec4(color.rgb, 1.0);
              vec2 ux = (up.xy * 255.0) / 256.0;
              vec2 uvf = fract(out_uv * console_size);
              vec2 uv = ux + uvf * uv_scale;
              vec4 col = texture2D(in_tex, uv);

              gl_FragColor = fg_color * col * col.a + bg_color * (1.0 - col.a);
        })gl"};

    std::pair<int, int> next_pos{0, 0};

    std::unordered_map<char32_t, uint32_t> char_uvs;
    gl_wrap::Texture font_texture;
    gl_wrap::Texture uv_texture;
    gl_wrap::Texture col_texture;
    gl_wrap::Program program;
    std::vector<uint32_t> data;
    std::vector<uint32_t> uvdata;
    std::vector<uint32_t> coldata;

    unsigned width;
    unsigned height;

public:
    PixConsole(unsigned w, unsigned h)
        : font{"data/bedstead.otf"}, width(w), height(h)
    {
        font.set_pixel_size(32);
        data.resize(texture_width * texture_height);
        std::tie(char_width, char_height) = font.get_size();

        char_height = (char_height + 1) & 0xfffffffe;
        char_width = (char_width + 1) & 0xfffffffe;

        std::fill(data.begin(), data.end(), 0);

        for (char32_t c = 0x20; c <= 0x7f; c++) {
            add_char(c);
        }
        font_texture = gl_wrap::Texture{texture_width, texture_height, data};

        uvdata.resize(w * h);
        std::fill(uvdata.begin(), uvdata.end(), 0);
        uv_texture = gl_wrap::Texture{w, h, uvdata};

        coldata.resize(w * h);
        std::fill(coldata.begin(), coldata.end(), 0);
        col_texture = gl_wrap::Texture{w, h, coldata};

        col_texture.bind(2);
        uv_texture.bind(1);
        font_texture.bind(0);

        program = gl_wrap::Program({vertex_shader}, {fragment_shader});

        program.setUniform("in_tex", 0);
        program.setUniform("uv_tex", 1);
        program.setUniform("col_tex", 2);

        program.setUniform("console_size", std::pair<float, float>(w, h));
        program.setUniform("uv_scale",
            std::pair<float, float>(static_cast<float>(char_width) /
                                        static_cast<float>(texture_width),
                static_cast<float>(char_height) /
                    static_cast<float>(texture_height)));
        for (auto& u : uvdata) {
            uint32_t x = char_uvs['!' + rand() % 0x40];
            x |= 0x01000000;
            u = x;
        }
        uv_texture.update(uvdata.data());

        for (auto& c : coldata) {
            c = 0x00FF00FF;
        }
        col_texture.update(coldata.data());
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
            t |= 0xFFFF'0000;
            x++;
            if (x >= width) {
                x = 0;
                y++;
            }
        }
    }

    static constexpr std::pair<uint32_t, uint32_t> make_col(
        uint32_t fg, uint32_t bg)
    {
        bg >>= 8;
        return std::pair<uint32_t, uint32_t>{
            fg & 0xffff0000, ((bg & 0xff) << 16) | (bg & 0xff00) | (bg >> 16) |
                                 ((fg << 16) & 0xff000000)};
    }

    void text(int x, int y, std::string const& t, uint32_t fg, uint32_t bg)
    {
        auto text32 = utils::utf8_decode(t);

        auto [w0, w1] = make_col(fg, bg);
        for (auto c : text32) {
            if (c == 10) {
                x = 0;
                y++;
                continue;
            }

            uvdata[x + width * y] = char_uvs[c] | w0;
            coldata[x + width * y] = w1;
            x++;
            if (x >= width) {
                x = 0;
                y++;
            }
        }
    }

    void flush()
    {
        uv_texture.update(uvdata.data());
        col_texture.update(coldata.data());
    }

    void put_char(int x, int y, char32_t c) {}

    void put_color(int x, int y, uint32_t fg, uint32_t bg)
    {
        auto [w0, w1] = make_col(fg, bg);
        uvdata[x + width * y] = (uvdata[x + width * y] & 0xffff) | w0;
        coldata[x + width * y] = w1;
    }

    void fill(uint32_t fg, uint32_t bg)
    {
        auto [w0, w1] = make_col(fg, bg);
        w0 |= char_uvs[' '];
        for (int i = 0; i < uvdata.size(); i++) {
            uvdata[i] = w0;
            coldata[i] = w1;
        }
    }

    void add_char(char32_t c)
    {
        auto cw = char_width + 2;
        // First render character into texture
        auto* ptr = &data[next_pos.first + next_pos.second * texture_width];
        int x = next_pos.first / 2;
        int y = next_pos.second / 2;
        font.render_char(c, ptr, 0xffffff00, texture_width);
        char_uvs[c] = (x) | (y << 8);

        next_pos.first += cw;
        if (next_pos.first >= (texture_width - cw)) {
            next_pos.first = 0;
            next_pos.second += (char_height + 2);
        }
    }

    float s = 1.0;
    float x = 0;

    void render()
    {
        // s *= 1.001;
        glDisable(GL_BLEND);
        col_texture.bind(2);
        uv_texture.bind(1);
        font_texture.bind(0);
        program.use();
        float w = s * static_cast<float>(width * char_width);
        float h = s * static_cast<float>(height * char_height);
        pix::draw_quad_impl(x, 0, w, h);
        x -= 1;
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

    // con.fill(0xffffffff, 0x004040ff);

    // con.text(2, 2, "ZweZxywXVwooPO");
    con.text(2, 3, "Hello citizens of the earth! This console is fast!",
        0x8080ffff, 0x0000ffff);
    con.flush();
    while (true) {

        auto event = system->poll_events();
        if (std::holds_alternative<QuitEvent>(event)) { break; }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        gl::clearColor(bg);
        glClear(GL_COLOR_BUFFER_BIT);
        con.render();

        window->swap();
    }
}
