#include <SDL.h>
#include <SDL2/SDL_opengl.h>

#ifdef __EMSCRIPTEN__
#    include <emscripten.h>
#endif

#include <functional>

namespace pix {
class Screen
{
    SDL_Window* window;

public:
    Screen(int w, int h)
    {
        SDL_Init(SDL_INIT_VIDEO);
        window = SDL_CreateWindow("SDL2Test", SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_OPENGL);
        SDL_GL_CreateContext(window);
    }

    explicit Screen(void* _window) : window(static_cast<SDL_Window*>(_window))
    {}

    virtual bool main_loop()
    {
        auto quit = loop_fn();
        return quit;
    }

    std::function<bool()> loop_fn{};

    void run()
    {
#ifdef __EMSCRIPTEN__
        emscripten_set_main_loop_arg(
            [](void* data) {
                auto* screen = static_cast<Screen*>(data);
                auto quit = screen->main_loop();
                // SDL_GL_SwapWindow(screen->window);
            },
            this, 0, true);
#else
        while (true) {
            if (main_loop()) { break; }
            //SDL_GL_SwapWindow(window);
        }
#endif
        // SDL_DestroyWindow(window);
        // SDL_Quit();
    }

    void run(std::function<bool()>&& fn)
    {
        loop_fn = std::move(fn);
        run();
    }
};
} // namespace pix
