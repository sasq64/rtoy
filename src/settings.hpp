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
    int display_width = 1440;
    int display_height = 960;
    std::string boot_cmd = "yo";
};

