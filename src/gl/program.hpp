#pragma once
#include "color.hpp"
#include "functions.hpp"
#include "gl.hpp"
#include "shader.hpp"

#include <cassert>
#include <vector>

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

    static inline Program& current()
    {
        static Program p;
        glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&p.program));
        return p;
    }

    ~Program()
    {
        if (program != 0) { glDeleteProgram(program); }
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

    static void glUniform(GLint location, std::pair<double, double> const& vec2)
    {
        glUniform2f(location, static_cast<float>(vec2.first),
            static_cast<float>(vec2.second));
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

    static void glUniform(GLint location, std::vector<Color> const& colors)
    {
        std::vector<float> data(colors.size() * 4);
        int i = 0;
        for (auto const& c : colors) {
            data[i++] = c.red;
            data[i++] = c.green;
            data[i++] = c.blue;
            data[i++] = c.alpha;
        }
        glUniform4fv(
            location, static_cast<GLsizei>(colors.size()), data.data());
    }

    static void glUniform(GLint location, float v) { glUniform1f(location, v); }
    static void glUniform(GLint location, double v)
    {
        glUniform1f(location, static_cast<float>(v));
    }

    static void glUniform(GLint location, int32_t v)
    {
        glUniform1i(location, v);
    }

    template <typename... ARGS>
    void setUniform(GLint location, ARGS... args) const
    {
        glUniform(location, args...);
    }
    template <typename... ARGS>
    void setUniform(const char* name, ARGS... args) const
    {
        auto location = glGetUniformLocation(program, name);
        if (location == -1) {
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
