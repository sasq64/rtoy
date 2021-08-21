#pragma once
#include "functions.hpp"
#include "gl.hpp"

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

    Texture() = default;

    void init();

    template <typename T, size_t N>
    Texture(GLuint w, GLuint h, std::array<T, N> const& data,
        GLint target_format = GL_RGBA, GLint source_format = -1,
        GLenum type = GL_UNSIGNED_BYTE)
        : width(w), height(h)
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
    Texture(GLuint w, GLuint h, std::vector<T> const& data,
        GLint target_format = GL_RGBA, GLint source_format = -1,
        GLenum type = GL_UNSIGNED_BYTE)
        : width(w), height(h)
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
    Texture(GLuint w, GLuint h, T const* data, GLint target_format = GL_RGBA,
        GLint source_format = -1, GLenum type = GL_UNSIGNED_BYTE)
        : width(w), height(h)
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

    Texture(GLuint w, GLuint h) : width(w), height(h)
    {
        init();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
            GL_UNSIGNED_BYTE, nullptr);
        glGenFramebuffers(1, &fb_id);
        glBindFramebuffer(GL_FRAMEBUFFER, fb_id);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_id, 0);
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

    void set_target() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fb_id);
        setViewport({width, height});
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

    std::pair<float, float> size() const { return {width, height}; }
};

struct TexRef
{
    std::shared_ptr<Texture> tex;
    std::array<float, 8> uvs{0.F, 0.F, 1.F, 0.F, 1.F, 1.F, 0.F, 1.F};
    void bind(int unit = 0) { tex->bind(unit); }
    bool operator==(TexRef const& other) const { return tex == other.tex; }
};

} // namespace gl_wrap
