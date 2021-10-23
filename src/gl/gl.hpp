
#pragma once

#ifdef USE_GLES
#include <GLES/gl.h>
#include <GLES2/gl2.h>
#else
#include <GL/glew.h>
#endif
#include <cstdint>
#include <type_traits>

namespace gl_wrap {

template <typename T>
constexpr typename std::enable_if_t<std::is_enum_v<T>, GLint> to_glenum(T f)
{
    return static_cast<GLint>(f);
}


template <typename T>
constexpr typename std::enable_if_t<std::is_arithmetic_v<T>, GLint> to_glint(T f)
{
    return static_cast<GLint>(f);
}

struct Attribute
{
    GLint location = -1;
    void enable() const { glEnableVertexAttribArray(location); }
    void disable() const { glDisableVertexAttribArray(location); }
};

} // namespace gl_wrap
