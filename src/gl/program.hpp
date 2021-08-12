#pragma once
#include "color.hpp"
#include "functions.hpp"
#include "gl.hpp"
#include "shader.hpp"

#include <cassert>

namespace gl_wrap {

struct Program
{
    GLuint program = 0;

    Program() { program = glCreateProgram(); }
    Program(Program const&) = delete;
    Program(Program&& other) noexcept
    {
        program = other.program;
        other.program = 0;
    }
    Program& operator=(Program const&) = delete;
    Program& operator=(Program&& other) noexcept
    {
        program = other.program;
        other.program = 0;
        return *this;
    }

    explicit Program(GLuint _program) : program(_program) {}

    ~Program()
    {
        if (program != 0) {
            glDeleteProgram(program);
        }
    }

    Program(VertexShader const& vs, FragmentShader const& fs)
    {
        program = glCreateProgram();
        vs.attach(program);
        fs.attach(program);
        glLinkProgram(program);
        gl_check("glLinkProgram");

        if (getProgrami(program, GL_LINK_STATUS) != GL_TRUE) {
            throw gl_exception("Linking failed");
        }
    }

    void use() const
    {
        glUseProgram(program);
        gl_check("glUseProgram");
    }

    GLint getAttribLocation(const char* name) const
    {
        auto rc = glGetAttribLocation(program, name);
        gl_check("glGetAttribLocation");
        return rc;
    }

    Attribute getAttribute(const char* name) const
    {
        return {getAttribLocation(name)};
    }

    static void glUniform(GLint location, std::pair<float, float> const& vec2)
    {
        glUniform2f(location, vec2.first, vec2.second);
    }

    static void glUniform(GLint location, std::array<float, 9> const& mat)
    {
        glUniformMatrix3fv(location, 1, GL_FALSE, mat.data());
    }

    static void glUniform(GLint location, std::array<float, 16> const& mat)
    {
        glUniformMatrix4fv(location, 1, GL_FALSE, mat.data());
    }

    static void glUniform(GLint location, std::array<float, 4> const& mat)
    {
        glUniformMatrix2fv(location, 1, GL_FALSE, mat.data());
    }

    static void glUniform(GLint location, Color const& color)
    {
        glUniform4f(location, color.red, color.green, color.blue, color.alpha);
    }

    static void glUniform(GLint location, float v) { 
        glUniform1f(location, v);
    }

    template <typename... ARGS>
    void setUniform(const char* name, ARGS... args) const
    {
        auto location = glGetUniformLocation(program, name);
        if(location == -1) {
            fmt::print("WARN: '{}' does not exist\n", name);
            return;
        }

        gl_check("glGetUniformLocation");
        use();
        glUniform(location, args...);
        gl_check("glUniform");
    }
};

} // namespace gl_wrap
