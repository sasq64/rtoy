#pragma once
#include "gl.hpp"
#include <array>
#include <cassert>
#include <cstddef>
#include <vector>

namespace gl_wrap {

enum class BufferTarget
{
    Array = GL_ARRAY_BUFFER,
    ElementArray = GL_ELEMENT_ARRAY_BUFFER,
};

enum class Usage
{
    Static = GL_STATIC_DRAW,
    Stream = GL_STREAM_DRAW
};

// Wraps a GL buffer. Also remembers the target and size
// of the buffer.
template <BufferTarget Target, GLenum Usage = GL_STATIC_DRAW>
struct Buffer
{
    GLuint buffer = 0;
    size_t size = 0;

    Buffer() = default;

    ~Buffer()
    {
        if (buffer != 0) {
            glDeleteBuffers(1, &buffer);
        }
    }

    explicit Buffer(size_t size_in_bytes = 0)
    {
        glGenBuffers(1, &buffer);
        bind();
        if (size_in_bytes != 0) {
            glBufferData(to_glenum(Target), size_in_bytes, nullptr, Usage);
            size = size_in_bytes;
        }
    }

    template <typename T, size_t N>
    explicit Buffer(std::array<T, N> const& data)
    {
        glGenBuffers(1, &buffer);
        bind();
        set(data);
    }

    template <typename T>
    explicit Buffer(std::vector<T> const& data)
    {
        glGenBuffers(1, &buffer);
        bind();
        set(data);
    }

    // Bind this buffer to its target
    void bind() const { glBindBuffer(to_glenum(Target), buffer); }

    // Bind this buffer and update part of its content
    void update(void* data, size_t offset_in_bytes, size_t size_in_bytes)
    {
        assert(size >= (offset_in_bytes + size_in_bytes));
        bind();
        glBufferSubData(to_glenum(Target), offset_in_bytes, size_in_bytes, data);
    }

    // Bind this buffer and update part of its content
    template <typename T>
    void update(std::vector<T> const& data, size_t offset_in_bytes)
    {
        assert(size >= (offset_in_bytes + data.size() * sizeof(T)));
        bind();
        glBufferSubData(to_glenum(Target), offset_in_bytes,
            data.size() * sizeof(T), data.data());
    }

    // Bind this buffer and reset its content
    void set(void* data, int size_in_bytes)
    {
        assert(size >= size_in_bytes);
        bind();
        glBufferData(to_glenum(Target), size_in_bytes, data, Usage);
        size = size_in_bytes;
    }

    // Bind this buffer and reset its content
    template <typename T, size_t N>
    void set(std::array<T, N> const& data)
    {
        glBufferData(
            to_glenum(Target), data.size() * sizeof(T), data.data(), Usage);
        size = data.size() * sizeof(T);
    }

    // Bind this buffer and reset its content
    template <typename T>
    void set(std::vector<T> const& data)
    {
        glBufferData(
            to_glenum(Target), data.size() * sizeof(T), data.data(), Usage);
        size = data.size() * sizeof(T);
    }
};

template <GLenum T = GL_STATIC_DRAW>
using ArrayBuffer = Buffer<BufferTarget::Array, T>;

template <GLenum T = GL_STATIC_DRAW>
using ElementBuffer = Buffer<BufferTarget::ElementArray, T>;

} // namespace gl_wrap
