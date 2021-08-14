#include "system.hpp"

#include <fmt/format.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <bcm_host.h>

#include <deque>
#include <filesystem>

namespace fs = std::filesystem;

bool initEGL(EGLConfig& eglConfig, EGLContext& eglContext,
    EGLDisplay& eglDisplay, EGLSurface& eglSurface,
    EGLNativeWindowType nativeWin);

class PiScreen : public Screen
{};

class SDLSystem : public System
{
    static EGL_DISPMANX_WINDOW_T nativewindow;

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
        if (display_width > 1024) {
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
    }

    void init_input(Settings const& settings) override
    {
        fs::path idir{"/dev/input"};
        std::vector<uint8_t> evbit((EV_MAX + 7) / 8);
        std::vector<uint8_t> keybit((KEY_MAX + 7) / 8);

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
                // LOGD("%s, %02x -- %02x", f.getName(), evbit, keybit);
            }
        }

        fmt::format("Found {} devices with keys\n", fdv.size());
    }

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

		vector<uint8_t> buf(256);
        if(!events.empty()) {
            auto e = events.front();
            e.pop_front();
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
            // LOGD("Got signal");
            // static uint8_t buf[2048];
            for (auto fd : fdv) {
                if (FD_ISSET(fd, &readset)) {
                    int rc = read(fd, &buf[0], sizeof(struct input_event) * 4);
                    auto* ptr = (struct input_event*)&buf[0];
                    // if(rc >= sizeof(struct input_event))
                    //	LOGD("[%02x]", buf);
                    while (rc >= sizeof(struct input_event)) {
                        if (ptr->type == EV_KEY) {
                            // LOGD("TYPE %d CODE %d VALUE %d", ptr->type,
                            // ptr->code, ptr->value);
                            if (ptr->value) {
                                auto k = ptr->code;
                                if (k >= KEY_1 && k <= KEY_9)
                                    k += ('1' - KEY_1);
                                else if (k >= KEY_F1 && k <= KEY_F10)
                                    k += (F1 - KEY_F1);
                                else {
                                    for (auto t : Window::translate) {
                                        if (t.second == k) {
                                            k = t.first;
                                            break;
                                        }
                                    }
                                }
                                putEvent<KeyEvent>(k);
                            }
                            //if (ptr->code < 512)
                            //    pressed_keys[ptr->code] = ptr->value;
                        }
                        ptr++;
                        rc -= sizeof(struct input_event);
                    }
                }
            }
        }
    }
};
