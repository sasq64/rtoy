#pragma once
#include "functions.hpp"
#include "gl.hpp"

#include <fmt/format.h>

#include <array>
#include <string_view>

namespace gl_wrap {

template <ShaderType ShaderType>
struct Shader
{
    GLuint shader = 0;
    GLint result = GL_FALSE;

    Shader() = default;
    Shader(Shader const&) = delete;
    Shader(Shader&& other) noexcept
    {
        shader = other.shader;
        other.shader = 0;
    }
    Shader& operator=(Shader const&) = delete;
    Shader& operator=(Shader&& other) noexcept
    {
        shader = other.shader;
        other.shader = 0;
        return *this;
    }
    ~Shader()
    {
        if (shader != 0) {
            glDeleteShader(shader);
        }
    }

    Shader(std::string_view src) // NOLINT
    {
        std::array sources{src.data()};
        std::array sizes{static_cast<GLint>(src.size())};
        shader = createShader(ShaderType);
        if (shader == 0) {
            throw gl_exception("glCreateShader");
        }
        glShaderSource(shader, 1, sources.data(), sizes.data());
        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &result);

        if (result != GL_TRUE) {
            auto info = getShaderInfoLog(shader);
            fmt::print("{}\n", info);
        }
    }

    void attach(GLuint program) const { glAttachShader(program, shader); }

    explicit operator bool() const { return result == GL_TRUE; }
};

using VertexShader = Shader<ShaderType::Vertex>;
using FragmentShader = Shader<ShaderType::Fragment>;

} // namespace gl_wrap
