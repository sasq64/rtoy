#pragma once
#include "functions.hpp"
#include "gl.hpp"
#include <cmath>
#include <fmt/core.h>

#include <array>
#include <memory>
#include <vector>

namespace gl_wrap {

struct Texture
{
    GLuint tex_id = 0;
    GLuint fb_id = 0;
    GLuint width = 0;
    GLuint height = 0;
    GLint format = GL_RGBA;

    Texture() = default;

    void init();

    template <typename T, size_t N>
    Texture(GLint w, GLint h, std::array<T, N> const& data,
        GLint target_format = GL_RGBA, GLint source_format = -1,
        GLenum type = GL_UNSIGNED_BYTE)
        : width(w), height(h), format(target_format)
    {
        if (source_format < 0) {
            constexpr static std::array translate{
                0, GL_ALPHA, 0, GL_RGB, GL_RGBA};
            source_format = translate[sizeof(T)];
        }
        init();
        glTexImage2D(GL_TEXTURE_2D, 0, target_format, w, h, 0,
            // Defines how many of the underlying elements form a pixel
            source_format,
            // Underlying type in array
            type, data.data());
    }

    template <typename T>
    Texture(GLint w, GLint h, std::vector<T> const& data,
        GLint target_format = GL_RGBA, GLint source_format = -1,
        GLenum type = GL_UNSIGNED_BYTE)
        : width(w), height(h), format(target_format)
    {
        if (source_format < 0) {
            constexpr static std::array translate{
                0, GL_ALPHA, 0, GL_RGB, GL_RGBA};
            source_format = translate[sizeof(T)];
        }
        init();
        glTexImage2D(GL_TEXTURE_2D, 0, target_format, w, h, 0,
            // Defines how many of the underlying elements form a pixel
            source_format,
            // Underlying type in array
            type, data.data());
    }

    template <typename T>
    Texture(GLint w, GLint h, T const* data, GLint target_format = GL_RGBA,
        GLint source_format = -1, GLenum type = GL_UNSIGNED_BYTE)
        : width(w), height(h), format(target_format)
    {
        init();
        if (source_format < 0) {
            constexpr static std::array translate{
                0, GL_ALPHA, 0, GL_RGB, GL_RGBA};
            source_format = translate[sizeof(T)];
        }
        glTexImage2D(GL_TEXTURE_2D, 0, target_format, w, h, 0,
            // Defines how many of the underlying elements form a pixel
            source_format,
            // Underlying type in array
            type, data);
    }

    Texture(GLint w, GLint h) : width(w), height(h)
    {
        init();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
            GL_UNSIGNED_BYTE, nullptr);
    }

    void fill(uint32_t col)
    {
        auto old = fb_id;
        set_target();
        clearColor({col});
        glClear(GL_COLOR_BUFFER_BIT);
        if (old == 0) { untarget(); }
    }

    void untarget()
    {
        glDeleteFramebuffers(1, &fb_id);
        fb_id = 0;
    }

    void move_from(Texture&& other) noexcept
    {
        tex_id = other.tex_id;
        fb_id = other.fb_id;
        width = other.width;
        height = other.height;
        other.tex_id = 0;
        other.fb_id = 0;
    }

    Texture(Texture const&) = delete;
    Texture(Texture&& other) noexcept { move_from(std::move(other)); }

    ~Texture()
    {
        if (tex_id != 0) { glDeleteTextures(1, &tex_id); }
        if (fb_id != 0) { glDeleteFramebuffers(1, &fb_id); }
    };

    Texture& operator=(Texture const&) = delete;

    Texture& operator=(Texture&& other) noexcept
    {
        move_from(std::move(other));
        return *this;
    }

    void bind(int unit = 0) const
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, tex_id);
    }

    void set_target()
    {
        if (fb_id == 0) {
            // glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex_id);
            glGenFramebuffers(1, &fb_id);
            glBindFramebuffer(GL_FRAMEBUFFER, fb_id);
            glFramebufferTexture2D(
                GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_id, 0);
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, fb_id);
        }
        setViewport({width, height});
    }

    std::vector<std::byte> read_pixels(
        int x = 0, int y = 0, int w = -1, int h = -1)
    {
        if (w < 0) { w = width; }
        if (h < 0) { h = height; }

        GLuint fb = 0;
        fmt::print("Read {},{} {}x{}\n", x, y, w, h);
        if (fb_id == 0) {
            glGenFramebuffers(1, &fb);
            glBindFramebuffer(GL_FRAMEBUFFER, fb);
            glFramebufferTexture2D(
                GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_id, 0);
            gl_check("glFrameBufferTexture2d");
            setViewport({width, height});
        } else {
            set_target();
        }
        std::vector<std::byte> data(width * height * 4);
        auto type = GL_UNSIGNED_BYTE;
        glReadPixels(x, y, w, h, format, type, data.data());
        gl_check("glReadPixels");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return data;
    }

    template <typename T>
    void update(
        T const* ptr, GLint source_format = -1, GLenum type = GL_UNSIGNED_BYTE)
    {
        if (source_format < 0) {
            constexpr static std::array translate{
                0, GL_ALPHA, 0, GL_RGB, GL_RGBA};
            source_format = translate[sizeof(T)];
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        glTexSubImage2D(
            GL_TEXTURE_2D, 0, 0, 0, width, height, source_format, type, ptr);
    }

    template <typename T>
    void update(int x, int y, int w, int h, T const* ptr,
        GLint source_format = -1, GLenum type = GL_UNSIGNED_BYTE)
    {
        if (source_format < 0) {
            constexpr static std::array translate{
                0, GL_ALPHA, 0, GL_RGB, GL_RGBA};
            source_format = translate[sizeof(T)];
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, source_format, type, ptr);
    }

    std::pair<float, float> size() const { return {width, height}; }
};

struct TexRef
{
    std::shared_ptr<Texture> tex;
    std::array<float, 8> uvs{0.F, 0.F, 1.F, 0.F, 1.F, 1.F, 0.F, 1.F};
    void bind(int unit = 0) { tex->bind(unit); }
    bool operator==(TexRef const& other) const { return tex == other.tex; }
    std::vector<std::byte> read_pixels()
    {
        return tex->read_pixels(x(), y(), width(), height());
    };

    TexRef() = default;

    TexRef(int w, int h) : tex{std::make_shared<Texture>(w, h)} {}

    explicit TexRef(std::shared_ptr<Texture> const& t) : tex(t) {}
    TexRef(std::shared_ptr<Texture> const& t, std::array<float, 8> const& u) :
        tex(t), uvs(u) {}

    std::vector<TexRef> split(int w, int h)
    {
        float u0 = uvs[0];
        float v0 = uvs[1];
        float u1 = uvs[4];
        float v1 = uvs[5];
        auto du = (u1 - u0) / static_cast<float>(w);
        auto dv = (v1 - v0) / static_cast<float>(h);

        std::vector<TexRef> images;

        float u = u0;
        float v = v0;
        int x = 0;
        int y = 0;
        while (true) {
            if (x == w) {
                u = u0;
                v += dv;
                x = 0;
                y++;
            }
            if (y == h) { break; }
            images.emplace_back(
                tex, std::array{u, v, u + du, v, u + du, v + dv, u, v + dv});

            u += du;
            x++;
        }
        return images;
    }

    void yflip()
    {
        auto y0 = uvs[1];
        auto y1 = uvs[5];
        uvs[1] = uvs[3] = y1;
        uvs[5] = uvs[7] = y0;
    }

    double width() const
    {
        return static_cast<double>(tex->width) * (uvs[4] - uvs[0]);
    }

    double height() const
    {
        return std::abs(static_cast<double>(tex->height) * (uvs[5] - uvs[1]));
    }
    double x() const { return tex->width * uvs[0]; }
    double y() const
    {
        auto uy = uvs[1];
        if ((uvs[5] - uy) < 0) { uy = 1.0F - uy; }
        return tex->height * uy;
    }
};

} // namespace gl_wrap
