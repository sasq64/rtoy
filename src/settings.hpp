#pragma once
#include <filesystem>

enum ScreenType
{
    Full,
    Window,
    None
};

struct Settings
{
    using path = std::filesystem::path;
    ScreenType screen = ScreenType::Window;
    path boot_script = "main.rb";
    path console_font = "data/unscii-16.ttf";
    int font_size = 16;
};

