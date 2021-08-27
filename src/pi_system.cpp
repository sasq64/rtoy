#include "system.hpp"
#include "keycodes.h"
#include <fmt/format.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <linux/input.h>
#include <fcntl.h>
#include <bcm_host.h>
#include <cctype>
#include <deque>
#include <filesystem>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

class display_exception : public std::exception {
public:
    display_exception(const std::string& msg) : msg(msg) {}
    virtual const char* what() const throw() { return msg.c_str(); }

private:
    std::string msg;
};

/*constexpr */ bool test_bit(const std::vector<uint8_t> &v, int n) {
	return (v[n/8] & (1<<(n%8))) != 0;
}

bool initEGL(EGLConfig& eglConfig, EGLContext& eglContext,
    EGLDisplay& eglDisplay, EGLSurface& eglSurface,
    EGLNativeWindowType nativeWin);

class PiScreen : public Screen
{
    EGLDisplay eglDisplay;
    EGLSurface eglSurface;
    int width;
    int height;
public:
    PiScreen(EGLDisplay d, EGLSurface s, int w, int h) : eglDisplay(d), eglSurface(s), width(w), height(h) {}
    void swap() override
    {
        if(eglDisplay != EGL_NO_DISPLAY) {
            eglSwapBuffers(eglDisplay, eglSurface);
            //eglQuerySurface(eglDisplay, eglSurface, EGL_WIDTH, &screenWidth);
            //eglQuerySurface(eglDisplay, eglSurface, EGL_HEIGHT, &screenHeight);
        }
    }
    std::pair<int, int> get_size() override
    {
        return {width, height};
    }
};

class PiSystem : public System
{
    EGL_DISPMANX_WINDOW_T nativewindow;

    EGLConfig eglConfig;
    EGLContext eglContext;
    EGLDisplay eglDisplay;
    EGLSurface eglSurface;

    std::vector<int> fdv;

public:
    std::shared_ptr<Screen> init_screen(Settings const& settings) override
    {
        bcm_host_init();

        DISPMANX_ELEMENT_HANDLE_T dispman_element;
        DISPMANX_DISPLAY_HANDLE_T dispman_display;
        DISPMANX_UPDATE_HANDLE_T dispman_update;
        VC_RECT_T dst_rect;
        VC_RECT_T src_rect;

        uint32_t display_width;
        uint32_t display_height;

        // create an EGL window surface, passing context width/height
        int success = graphics_get_display_size(
            0 /* LCD */, &display_width, &display_height);
        if (success < 0) throw display_exception("Cound not get display size");

        dst_rect.x = 0;
        dst_rect.y = 0;
        dst_rect.width = display_width;
        dst_rect.height = display_height;

        uint16_t dwa = 0;
        uint16_t dha = 0;

        // Scale 50% on hires screens
        if (display_width > 2024) {
            display_width /= 2;
            display_height /= 2;
            dwa = display_width;
            dha = display_height;
        }

        src_rect.x = 0;
        src_rect.y = 0;
        src_rect.width = display_width << 16 | dwa;
        src_rect.height = display_height << 16 | dha;

        dispman_display = vc_dispmanx_display_open(0 /* LCD */);
        dispman_update = vc_dispmanx_update_start(0);

        dispman_element = vc_dispmanx_element_add(dispman_update,
            dispman_display, 0 /*layer*/, &dst_rect, 0 /*src*/, &src_rect,
            DISPMANX_PROTECTION_NONE, nullptr /*alpha*/, nullptr /*clamp*/,
            DISPMANX_NO_ROTATE);

        nativewindow.element = dispman_element;
        nativewindow.width = display_width;
        nativewindow.height = display_height;
        vc_dispmanx_update_submit_sync(dispman_update);

        initEGL(eglConfig, eglContext, eglDisplay, eglSurface, &nativewindow);
        return std::make_shared<PiScreen>(eglDisplay, eglSurface, display_width, display_height);
    }

    void init_input(Settings const& settings) override
    {
        fs::path idir{"/dev/input"};
        std::vector<uint8_t> evbit((EV_MAX + 7) / 8);
        std::vector<uint8_t> keybit((KEY_MAX + 7) / 8);
        int keyboardCount = 0;

        int fd = -1;
        if (fs::is_directory(idir)) {
            for (auto&& p : fs::directory_iterator(idir)) {
                auto real_path = idir / p.path().filename();
                if (!fs::is_directory(real_path)) {
                    fd = ::open(real_path.c_str(), O_RDONLY, 0);
                    if (fd >= 0) {
                        ioctl(fd, EVIOCGBIT(0, evbit.size()), &evbit[0]);
                        if (test_bit(evbit, EV_KEY)) {
                            ioctl(fd, EVIOCGBIT(EV_KEY, keybit.size()),
                                &keybit[0]);
                            if (test_bit(keybit, KEY_F8)) keyboardCount++;
                            if (test_bit(keybit, KEY_LEFT) ||
                                test_bit(keybit, BTN_LEFT)) {
                                ioctl(fd, EVIOCGRAB, 1);
                                fdv.push_back(fd);
                                continue;
                            }
                        }
                        ::close(fd);
                    }
                }
                //fmt::print("{}, {:02x} -- {:02x}\n", p.path().string(), evbit, keybit);
            }
        }

        fmt::print("Found {} devices with keys\n", fdv.size());
    }
    static inline std::unordered_map<int, int> translate = {
        { 'a', KEY_A },
        { 'b', KEY_B },
        { 'c', KEY_C },
        { 'd', KEY_D },
        { 'e', KEY_E },
        { 'f', KEY_F },
        { 'g', KEY_G },
        { 'h', KEY_H },
        { 'i', KEY_I },
        { 'j', KEY_J },
        { 'k', KEY_K },
        { 'l', KEY_L },
        { 'm', KEY_M },
        { 'n', KEY_N },
        { 'o', KEY_O },
        { 'p', KEY_P },
        { 'q', KEY_Q },
        { 'r', KEY_R },
        { 's', KEY_S },
        { 't', KEY_T },
        { 'u', KEY_U },
        { 'v', KEY_V },
        { 'w', KEY_W },
        { 'x', KEY_X },
        { 'y', KEY_Y },
        { 'z', KEY_Z },
        { '0', KEY_0 },

        { '-', KEY_MINUS },
        { '=', KEY_EQUAL },
        { '[', KEY_LEFTBRACE },
        { ']', KEY_LEFTBRACE },
        { '\\', KEY_BACKSLASH },
        { ';', KEY_SEMICOLON },
        { ',', KEY_COMMA },
        { '.', KEY_DOT },
        { '/', KEY_SLASH },
        { '\'', KEY_APOSTROPHE },
        { RKEY_F11, KEY_F11 },
        { RKEY_F12, KEY_F12 },
//        { BTN_LEFT, CLICK },
//        { BTN_RIGHT, RIGHT_CLICK },
        { RKEY_ENTER, KEY_ENTER },
        { RKEY_SPACE, KEY_SPACE },
        { RKEY_PAGEUP, KEY_PAGEUP },
        { RKEY_PAGEDOWN, KEY_PAGEDOWN },
        { RKEY_RIGHT, KEY_RIGHT },
        { RKEY_LEFT, KEY_LEFT },
        { RKEY_DOWN, KEY_DOWN },
        { RKEY_UP, KEY_UP },
        { RKEY_ESCAPE, KEY_ESC },
        { RKEY_BACKSPACE, KEY_BACKSPACE },
        { RKEY_DELETE, KEY_DELETE },
        { RKEY_TAB, KEY_TAB },
        { RKEY_END, KEY_END },
        { RKEY_HOME, KEY_HOME },
        { RKEY_LSHIFT, KEY_LEFTSHIFT },
        { RKEY_RSHIFT, KEY_RIGHTSHIFT },
        { RKEY_LWIN, KEY_LEFTMETA },
        { RKEY_RWIN, KEY_RIGHTMETA },
        { RKEY_LALT, KEY_LEFTALT },
        { RKEY_RALT, KEY_RIGHTALT },
        { RKEY_LCTRL, KEY_LEFTCTRL},
        { RKEY_RCTRL, KEY_RIGHTCTRL }
    };

    std::unordered_map<uint32_t, uint32_t> mappings;

    void map_key(uint32_t code, uint32_t target, int mods) override
    {
        mappings[code | (mods<<24)] = target;
    }

    bool shift_down = false;

    std::deque<AnyEvent> events;

    template <typename E>
    void putEvent(E const& e)
    {
        events.push_back(e);
    }

    AnyEvent poll_events() override
    {
		int maxfd = -1;

		fd_set readset;
		struct timeval tv;

        std::vector<uint8_t> buf(256);
        if(!events.empty()) {
            auto e = events.front();
            events.pop_front();
            return e;
        }
        FD_ZERO(&readset);
        for (auto fd : fdv) {
            FD_SET(fd, &readset);
            if (fd > maxfd) maxfd = fd;
        }
        tv.tv_sec = 0;
        tv.tv_usec = 1000;
        int sr = select(maxfd + 1, &readset, nullptr, nullptr, &tv);
        if (sr > 0) {
            fmt::print("Got signal\n");
            // static uint8_t buf[2048];
            for (auto fd : fdv) {
                if (FD_ISSET(fd, &readset)) {
                    int rc = read(fd, &buf[0], sizeof(struct input_event) * 4);
                    auto* ptr = (struct input_event*)&buf[0];


                    while (rc >= sizeof(struct input_event)) {
                        fmt::print("TYPE {} CODE {} VALUE {}\n", ptr->type, ptr->code, ptr->value);
                        if (ptr->type == EV_KEY) {
                            uint32_t k = ptr->code;
                            if (k == KEY_LEFTSHIFT || k == KEY_RIGHTSHIFT) {
                                shift_down = ptr->value != 0;
                            }
                            if (ptr->value) {

                                auto it = mappings.find(k | mods);
                                if(it != mappings.end()) {
                                    k = it->second;
                                    fmt::print("Converted to {}\n", k);
                                    putEvent(KeyEvent{k, 0});
                                }

                                if (k >= KEY_1 && k <= KEY_9) {
                                    k += ('1' - KEY_1);
                                } else if (k >= KEY_F1 && k <= KEY_F10) {
                                    fmt::print("F ");
                                    k = k + RKEY_F1 - KEY_F1;
                                } else {
                                    for (auto t : translate) {
                                        if (t.second == k) {
                                            k = t.first;
                                            break;
                                        }
                                    }
                                }
                                if (shift_down) {
                                    k = toupper(k);
                                    if(k == '\'') k = '\"';
                                }
                                fmt::print("Converted to {}\n", k);
                                putEvent(KeyEvent{k, 0});
                            }
                        }
                        ptr++;
                        rc -= sizeof(struct input_event);
                    }
                }
            }
        }
        if(!events.empty()) {
            auto e = events.front();
            events.pop_front();
            return e;
        }
        return NoEvent{};
    }
};

std::unique_ptr<System> create_pi_system()
{
    return std::make_unique<PiSystem>();
}

