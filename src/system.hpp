#pragma once

#include "keycodes.h"
#include "settings.hpp"

#include <functional>
#include <tuple>
#include <unordered_map>
#include <variant>

struct KeyEvent
{
    uint32_t key;
    uint32_t mods;
};

struct NoEvent
{};

struct QuitEvent
{};

struct ClickEvent
{
    int x;
    int y;
    int buttons;
};

struct MoveEvent
{
    int x;
    int y;
    int buttons;
};

struct TextEvent
{
    std::string text;
};

using AnyEvent = std::variant<NoEvent, KeyEvent, MoveEvent, ClickEvent,
    TextEvent, QuitEvent>;

class Screen
{
public:
    virtual ~Screen() = default;
    virtual void swap() {}
    virtual std::pair<int, int> get_size() { return {-1, -1}; }
};

class System
{
public:
    virtual ~System() = default;
    virtual std::shared_ptr<Screen> init_screen(Settings const&)
    {
        return nullptr;
    }
    virtual void init_audio(Settings const&) {}
    virtual AnyEvent poll_events() { return NoEvent{}; }
    virtual void set_audio_callback(std::function<void(float*, size_t)> const&)
    {}

    virtual void init_input(Settings const&) {}
    virtual bool is_pressed(uint32_t code) { return false; }

    virtual void map_key(uint32_t code, uint32_t target, int mods) {}
};

std::unique_ptr<System> create_sdl_system();
std::unique_ptr<System> create_pi_system();
