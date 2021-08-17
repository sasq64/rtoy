#include "system.hpp"

#include "gl/gl.hpp"
#include <coreutils/utf8.h>

#include <SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_video.h>
#include <fmt/format.h>

class SDLWindow : public Screen
{
    SDL_Window* window = nullptr;

public:
    explicit SDLWindow(SDL_Window* win) : window(win) {}
    void swap() override { SDL_GL_SwapWindow(window); }
    std::pair<int, int> get_size() override
    {
        int w = -1;
        int h = -1;
        SDL_GL_GetDrawableSize(window, &w, &h);
        return {w, h};
    }
};

class SDLSystem : public System
{
    uint32_t dev = 0;
    std::unordered_map<uint32_t, int> pressed;

public:
    std::shared_ptr<Screen> init_screen(Settings const& settings) override
    {

        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
        auto* window = SDL_CreateWindow("Toy", SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
#ifdef OSX
            w / 2, h / 2,
            SDL_WINDOW_ALLOW_HIGHDPI |
#else
            settings.display_width, settings.display_height,
#endif
                SDL_WINDOW_OPENGL |
                (settings.screen == ScreenType::Full
                        ? SDL_WINDOW_FULLSCREEN_DESKTOP
                        : 0));
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_CreateContext(window);
        GLenum err = glewInit();
        return std::make_shared<SDLWindow>(window);
    }

    void init_input(Settings const& settings) override {}

    static uint32_t sdl2key(uint32_t code)
    {
        switch (code) {
        case SDLK_LEFT: return RKEY_LEFT;
        case SDLK_RIGHT: return RKEY_RIGHT;
        case SDLK_PAGEUP: return RKEY_PAGEUP;
        case SDLK_PAGEDOWN: return RKEY_PAGEDOWN;
        case SDLK_UP: return RKEY_UP;
        case SDLK_DOWN: return RKEY_DOWN;
        case SDLK_END: return RKEY_END;
        case SDLK_HOME: return RKEY_HOME;
        case SDLK_ESCAPE: return RKEY_ESCAPE;
        case SDLK_RETURN: return RKEY_ENTER;
        case SDLK_INSERT: return RKEY_INSERT;
        case SDLK_DELETE: return RKEY_DELETE;
        case SDLK_F1: return RKEY_F1;
        case SDLK_F5: return RKEY_F5;
        case SDLK_F7: return RKEY_F7;
        case SDLK_F3: return RKEY_F3;
        case SDLK_F12: return RKEY_F12;
        default: return code;
        }
    }

    bool is_pressed(uint32_t code) { return pressed[code] != 0; }

    AnyEvent poll_events() override
    {
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_MOUSEMOTION) {
                auto& me = e.motion;
                if (me.state != 0) {
                    fmt::print("{:x} {} {}\n", me.state, me.x, me.y);
                    return MoveEvent{me.x, me.y, 1};
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                return ClickEvent{e.button.x, e.button.y, 1};
            } else if (e.type == SDL_TEXTINPUT) {
                fmt::print("TEXT '{}'\n", e.text.text);
                return TextEvent{e.text.text};
            } else if (e.type == SDL_KEYDOWN) {
                auto code = sdl2key(e.key.keysym.sym);

                auto& ke = e.key;
                pressed[code] = 1;
                auto mod = ke.keysym.mod;
                // If code is non-ascii
                if (code < 0x20 || code > 0x1fffc || (mod & 0xc0) != 0) {
                    fmt::print("KEY {:x} MOD {:x}\n", ke.keysym.sym, mod);
                    return KeyEvent{code, mod};
                }
            } else if (e.type == SDL_KEYUP) {
                auto& ke = e.key;
                auto code = sdl2key(ke.keysym.sym);
                pressed[code] = 0;
            } else if (e.type == SDL_QUIT) {
                fmt::print("quit\n");
                return QuitEvent{};
            } else if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                    SDL_Log("Window %d resized to %dx%d", e.window.windowID,
                        e.window.data1, e.window.data2);
                }
            }
        }
        return NoEvent{};
    }

    std::function<void(float*, size_t)> audio_callback;

    void set_audio_callback(
        std::function<void(float*, size_t)> const& cb) override
    {
        audio_callback = cb;
    }

    void init_audio(Settings const& settings) override
    {
        SDL_AudioSpec want;
        SDL_AudioSpec have;

        SDL_memset(&want, 0, sizeof(want)); /* or SDL_zero(want) */
        want.freq = 44100;
        want.format = AUDIO_F32;
        want.channels = 2;
        want.samples = 4096;
        want.userdata = this;
        want.callback = [](void* userdata, Uint8* stream, int len) {
            auto* sys = static_cast<SDLSystem*>(userdata);
            if (sys->audio_callback) {
                sys->audio_callback(reinterpret_cast<float*>(stream), len / 4);
            }
        };
        dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have,
            SDL_AUDIO_ALLOW_ANY_CHANGE & (~SDL_AUDIO_ALLOW_FORMAT_CHANGE));

        fmt::print("Audio format {} {} vs {}\n", have.format, have.freq, dev);
        SDL_PauseAudioDevice(dev, 0);
    }
};

std::unique_ptr<System> create_sdl_system()
{
    return std::make_unique<SDLSystem>();
}

