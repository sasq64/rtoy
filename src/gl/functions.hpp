#pragma once
#include "color.hpp"
#include "gl.hpp"

#include <array>
#include <cassert>
#include <string>
#include <tuple>

namespace gl_wrap {

class gl_exception : public std::exception
{
public:
    explicit gl_exception(std::string m = "GL Exception") : msg(std::move(m)) {}
    const char* what() const noexcept override { return msg.c_str(); }

private:
    std::string msg;
};

inline char const* gl_error_string(GLenum const err) noexcept
{
    switch (err) {
    case GL_NO_ERROR: return "GL_NO_ERROR";
    case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
    case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
    case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
    case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    default: return "UNKNOWN_ERROR";
    }
}

inline void gl_check(std::string const& f)
{
#ifndef NDEBUG
    if (auto err = glGetError(); err != GL_NO_ERROR) {
        puts((f + "() error:" + gl_error_string(err)).c_str());
        fflush(stdout);
        // throw gl_exception(f + "() error:" + gl_error_string(err));
    }
#endif
}

template <typename T>
inline constexpr T gl_check(T const& res, std::string const& f)
{
#ifndef NDEBUG
    if (auto err = glGetError(); err != GL_NO_ERROR) {
        // throw gl_exception(f + "() error:" + gl_error_string(err));
        puts((f + "() error:" + gl_error_string(err)).c_str());
        fflush(stdout);
    }
#endif
    return res;
}

enum class ShaderType
{
    Vertex = GL_VERTEX_SHADER,
    Fragment = GL_FRAGMENT_SHADER,
};

inline GLint createShader(ShaderType shaderType)
{
    return gl_check(glCreateShader(to_glenum(shaderType)), "glCreateShader");
}

inline void clearColor(Color const& color)
{
    glClearColor(color.red, color.green, color.blue, color.alpha);
    gl_check("glClearColor");
}

inline void setViewport(std::pair<GLint, GLint> wh)
{
    glViewport(0, 0, wh.first, wh.second);
    gl_check("glViewport");
}

template <typename T = GLint>
inline std::pair<T, T> getViewport()
{
    std::array<GLint, 4> data; // NOLINT
    glGetIntegerv(GL_VIEWPORT, data.data());
    gl_check("glGetInteger");
    return {static_cast<T>(data[2]), static_cast<T>(data[3])};
}

inline GLint getShaderi(GLuint shader, GLenum what)
{
    GLint res{};
    glGetShaderiv(shader, what, &res);
    gl_check("glGetShader");
    return res;
}

inline GLint getProgrami(GLuint program, GLenum what)
{
    GLint res{};
    glGetProgramiv(program, what, &res);
    gl_check("glGetShader");
    return res;
}

inline std::string getShaderInfoLog(GLuint shader)
{
    auto len = getShaderi(shader, GL_INFO_LOG_LENGTH);
    std::string info;
    info.resize(len);
    glGetShaderInfoLog(shader, len, &len, info.data());
    gl_check("glGetShaderInfoLog");
    return info;
}

enum class Primitive
{
    TriangleFan = GL_TRIANGLE_FAN,
    TriangleStrip = GL_TRIANGLE_STRIP,
    Triangles = GL_TRIANGLES,
    Lines = GL_LINES,
    LineLoop = GL_LINE_LOOP,
};

enum class Type
{
    UnsignedInt = GL_UNSIGNED_INT,
    UnsignedShort = GL_UNSIGNED_SHORT,
    Float = GL_FLOAT,
};

inline void drawArrays(Primitive p, GLint offset, int count)
{
    glDrawArrays(to_glenum(p), offset, count);
    gl_check("glDrawArrays");
}

template <typename T>
void* to_ptr(T t) {
    return reinterpret_cast<void*>(static_cast<uintptr_t>(t));
}

inline void drawElements(Primitive p, int count, Type t, uintptr_t offset)
{
    glDrawElements(
        to_glenum(p), count, to_glenum(t), to_ptr(offset));
    gl_check("glDrawElements");
}

inline void vertexAttrib(GLuint index, GLint size, GLenum type, GLboolean norm,
    GLsizei stride, uintptr_t offset)
{
    assert(index < GL_MAX_VERTEX_ATTRIBS);
    assert(size > 0 && size <= 4);

    glVertexAttribPointer(
        index, size, type, norm, stride, to_ptr(offset));
    gl_check("glVertexAttribPointer");
}

template <int N>
struct Size
{
    static_assert(N > 0 && N < 5);
};

template <int N>
inline void vertexAttrib(
    GLuint index, Size<N>, Type type, GLsizei stride, GLuint offset)
{
    vertexAttrib(index, N, to_glenum(type), GL_FALSE, stride, offset);
    gl_check("glVertexAttrib");
}

inline void vertexAttrib(
    Attribute attr, GLint size, Type type, GLsizei stride, GLuint offset)
{
    vertexAttrib(
        attr.location, size, to_glenum(type), GL_FALSE, stride, offset);
    gl_check("glVertexAttrib");
}

#if 0
template <size_t N>
std::array<GLuint, N> genVertexArrays()
{
    std::array<GLuint, N> res;
    glGenVertexArrays(N, res.data());
    return res;
}

inline GLuint genVertexArray()
{
    GLuint res; // NOLINT
    glGenVertexArrays(1, &res);
    return res;
}
#endif

} // namespace gl_wrap
