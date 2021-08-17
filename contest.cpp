
#include "src/system.hpp"
#include <gl/color.hpp>
#include <gl/functions.hpp>
#include <gl/gl.hpp>
#include <gl/program_cache.hpp>
#include <gl/texture.hpp>

#include <pix/pix.hpp>

#include <variant>

namespace gl = gl_wrap;

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

    std::vector<unsigned> data(32 * 32);

    std::array<float, 16> transform{
        0.5, 0, 0, 0, 0, 0.5, 0, 0, 0, 0, 0.5, 0, 0, 0, 0, 1};
    gl::Texture texture{
        16, 16, data, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT};

    texture.bind();

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    while (true) {
        auto event = system->poll_events();
        if (std::holds_alternative<QuitEvent>(event)) { break; }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        gl::clearColor(bg);
        glClear(GL_COLOR_BUFFER_BIT);
        auto& program = gl::ProgramCache::get_instance().textured;
        program.use();
        glEnable(GL_BLEND);
        pix::set_transform(transform);
        pix::set_colors(0xffffffff, 0);
        auto [w, h] = gl::getViewport();
        pix::draw_quad();

        window->swap();
    }
}
