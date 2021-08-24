
#include "src/system.hpp"
#include "src/pixel_console.hpp"

#include <gl/color.hpp>
#include <gl/functions.hpp>
#include <gl/gl.hpp>
#include <gl/program.hpp>
#include <gl/texture.hpp>
#include <pix/font.hpp>
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

    con.set_tile_size(38, 38);
    con.fill(0xffffffff, 0x004040ff);

    auto image = pix::load_png("data/ball.png");
    con.set_tile_image('i', image);
    con.set_tile_image('H', image);

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
