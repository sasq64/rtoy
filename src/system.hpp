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

class Screen
{
public:
    virtual void swap() {}
    virtual std::pair<int, int> get_size() { return {-1, -1}; }
};

class System
{
public:
    virtual std::shared_ptr<Screen> init_screen(Settings const&)
    {
        return nullptr;
    }
    virtual void init_audio(Settings const& settings) {}
    virtual std::variant<NoEvent, KeyEvent, MoveEvent, ClickEvent, TextEvent>
    poll_events()
    {
        return NoEvent{};
    }
};

std::unique_ptr<System> create_sdl_system();
