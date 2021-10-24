#include "gl/gl.hpp"
#ifdef USE_ASOUND
#    include "player_linux.h"
#endif
#include "system.hpp"
#include <coreutils/utf8.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL_video.h>
#include <fmt/format.h>

#include <memory>
#include <unordered_map>
#include <vector>

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
    std::unordered_map<uint32_t, uint32_t> pressed;

    std::vector<SDL_Joystick*> joysticks;

#ifdef USE_ASOUND
    std::unique_ptr<LinuxPlayer> player;
#endif

public:


    SDLSystem()
    {
        fmt::format("SDL Init\n");
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);
    };

    std::shared_ptr<Screen> init_screen(Settings const& settings) override
    {

        SDL_JoystickEventState(SDL_ENABLE);
        for (int i = 0; i < SDL_NumJoysticks(); i++) {
            joysticks.push_back(SDL_JoystickOpen(i));
        }

        auto* window = SDL_CreateWindow("Toy", SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
#ifdef OSX
            w / 2, h / 2,
            SDL_WINDOW_ALLOW_HIGHDPI |
#else
            settings.display_width, settings.display_height,
#endif
                (settings.screen == ScreenType::None ? SDL_WINDOW_HIDDEN : 0) |
                SDL_WINDOW_OPENGL |
                (settings.screen == ScreenType::Full
                        ? SDL_WINDOW_FULLSCREEN_DESKTOP
                        : 0));
        // SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        // SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_CreateContext(window);
#ifndef USE_GLES
        GLenum err = glewInit();
#endif
        return std::make_shared<SDLWindow>(window);
    }

    void init_input(Settings const& settings) override {}

    static inline std::unordered_map<uint32_t, uint32_t> sdl_map = {
        {SDLK_LEFT, RKEY_LEFT},
        {SDLK_RIGHT, RKEY_RIGHT},
        {SDLK_PAGEUP, RKEY_PAGEUP},
        {SDLK_PAGEDOWN, RKEY_PAGEDOWN},
        {SDLK_UP, RKEY_UP},
        {SDLK_DOWN, RKEY_DOWN},
        {SDLK_END, RKEY_END},
        {SDLK_HOME, RKEY_HOME},
        {SDLK_ESCAPE, RKEY_ESCAPE},
        {SDLK_RETURN, RKEY_ENTER},
        {SDLK_INSERT, RKEY_INSERT},
        {SDLK_DELETE, RKEY_DELETE},
        {SDLK_F1, RKEY_F1},
        {SDLK_F2, RKEY_F2},
        {SDLK_F3, RKEY_F3},
        {SDLK_F4, RKEY_F4},
        {SDLK_F5, RKEY_F5},
        {SDLK_F6, RKEY_F6},
        {SDLK_F7, RKEY_F7},
        {SDLK_F8, RKEY_F8},
        {SDLK_F9, RKEY_F9},
        {SDLK_F10, RKEY_F10},
        {SDLK_F11, RKEY_F11},
        {SDLK_F12, RKEY_F12},
    };

    static constexpr bool in_unicode_range(uint32_t c)
    {
        return c >= 0x20 && c <= 0x0f'ffff;
    }

    static uint32_t sdl2key(uint32_t code)
    {
        auto it = sdl_map.find(code);
        if (it != sdl_map.end()) { return it->second; }
        return code;
    }

    bool is_pressed(uint32_t code, int device) override
    {
        if (device == -1) { return pressed[code] != 0; }
        return (pressed[code] & (1 << device)) != 0;
    }

    std::array<uint32_t, 16> lastAxis{};

    AnyEvent poll_events() override
    {
        static constexpr std::array jbuttons{RKEY_FIRE, RKEY_B, RKEY_X, RKEY_Y,
            RKEY_L1, RKEY_L2, RKEY_SELECT, RKEY_START};
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            int device = 0;
            if (e.type == SDL_MOUSEMOTION) {
                auto& me = e.motion;
                if (me.state != 0) {
                    fmt::print("{:x} {} {}\n", me.state, me.x, me.y);
                    return MoveEvent{me.x, me.y, 1};
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                return ClickEvent{e.button.x, e.button.y, 1};
            } else if (e.type == SDL_TEXTINPUT) {
                //fmt::print("TEXT '{}'\n", e.text.text);
                return TextEvent{e.text.text, 0};
            } else if (e.type == SDL_KEYDOWN) {
                auto code = sdl2key(e.key.keysym.sym);

                auto& ke = e.key;
                pressed[code] |= (1 << device);
                auto mod = ke.keysym.mod;
                if (!in_unicode_range(code) || (mod & 0xc0) != 0) {
                    //fmt::print("KEY {:x} MOD {:x}\n", ke.keysym.sym, mod);
                    return KeyEvent{code, mod, device};
                }
            } else if (e.type == SDL_KEYUP) {
                auto& ke = e.key;
                auto code = sdl2key(ke.keysym.sym);
                pressed[code] &= ~(1 << device);
            } else if (e.type == SDL_JOYBUTTONDOWN) {
                device = (e.jbutton.which + 1);
                fmt::print("JBUTTON {:x}\n", e.jbutton.button);
                if (e.jbutton.button <= 7) {
                    auto code = jbuttons[e.jbutton.button];
                    pressed[code] |= (1 << device);
                    return KeyEvent{code, 0, device};
                }
            } else if (e.type == SDL_JOYBUTTONUP) {
                device = (e.jbutton.which + 1);
                if (e.jbutton.button <= 7) {
                    auto code = jbuttons[e.jbutton.button];
                    pressed[code] &= ~(1 << device);
                }
            } else if (e.type == SDL_JOYAXISMOTION) {
                device = (e.jbutton.which + 1);
                uint32_t mask = (1 << device);
                fmt::print("JAXIS {} {} {}\n", e.jaxis.axis, e.jaxis.type,
                    e.jaxis.value);
                uint32_t neg_key = 0;
                uint32_t pos_key = 0;
                if (e.jaxis.axis == 0) {
                    neg_key = RKEY_LEFT;
                    pos_key = RKEY_RIGHT;
                } else if (e.jaxis.axis == 1) {
                    neg_key = RKEY_UP;
                    pos_key = RKEY_DOWN;
                }
                if (pos_key != 0) {
                    if (e.jaxis.value > 10000) {
                        if ((pressed[pos_key] & mask) == 0) {
                            pressed[pos_key] |= mask;
                            return KeyEvent{pos_key, 0, device};
                        }
                    } else if (e.jaxis.value < -10000) {
                        if ((pressed[neg_key] & mask) == 0) {
                            pressed[neg_key] |= mask;
                            return KeyEvent{neg_key, 0, device};
                        }
                    } else {
                        pressed[pos_key] &= ~mask;
                        pressed[neg_key] &= ~mask;
                    }
                }
            } else if (e.type == SDL_JOYHATMOTION) {
                fmt::print(
                    "JHAT {} {} {}\n", e.jhat.hat, e.jhat.value, e.jhat.which);
                static constexpr std::array directions{
                    RKEY_UP, RKEY_RIGHT, RKEY_DOWN, RKEY_LEFT};
                auto a = e.jhat.value;
                device = (e.jbutton.which + 1);
                auto delta = lastAxis[device] ^ a;
                uint32_t button = 0;
                bool down = false;
                for (int i = 0; i < 4; i++) {
                    uint32_t mask = 1 << i;
                    if ((delta & mask) == mask) {
                        button = directions[i];
                        down = (a & mask) != 0;
                    }
                }
                lastAxis[device] = a;
                if (down) {
                    pressed[button] |= (1 << device);
                    return KeyEvent{button, 0, device};
                }
                pressed[button] &= ~(1 << device);
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
#ifdef USE_ASOUND
    void init_audio(Settings const&) override
    {
        player = std::make_unique<LinuxPlayer>(44100);
    }

    void set_audio_callback(
        std::function<void(float*, size_t)> const& fcb) override
    {

        player->play([fcb](int16_t* data, size_t sz) {
            std::array<float, 32768> fa; // NOLINT
            fcb(fa.data(), sz);
            for (int i = 0; i < sz; i++) {
                auto f = std::clamp(fa[i], -1.0F, 1.0F);
                data[i] = static_cast<int16_t>(f * 32767.0);
            }
        });
    }
#else
    std::function<void(float*, size_t)> audio_callback;

    void set_audio_callback(
        std::function<void(float*, size_t)> const& cb) override
    {
        audio_callback = cb;
    }

    void init_audio(Settings const& /*settings*/) override
    {
        SDL_AudioSpec want;
        SDL_AudioSpec have;

        SDL_memset(&want, 0, sizeof(want));
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
            SDL_AUDIO_ALLOW_ANY_CHANGE);// & (~SDL_AUDIO_ALLOW_FORMAT_CHANGE));

        fmt::print("Audio format {} {} vs {}\n", have.format, have.freq, dev);
        SDL_PauseAudioDevice(dev, 0);
    }
#endif
};

std::unique_ptr<System> create_sdl_system()
{
    return std::make_unique<SDLSystem>();
}
