
#pragma once
#include "functions.hpp"
#include "gl.hpp"
#include "program.hpp"

#include <cassert>
#include <string>
#include <string_view>

namespace gl_wrap {

struct ProgramCache
{
    Program non_textured;
    Program textured;

    std::string vertex_shader{R"gl( 
    #ifdef GL_ES
        precision mediump float;
    #endif
        attribute vec2 in_pos;
        uniform mat4 in_transform;
        #ifdef TEXTURED
          attribute vec2 in_uv;
          varying vec2 out_uv;
        #endif
        void main() {
            vec4 v = in_transform * vec4(in_pos, 0, 1);
            gl_Position = vec4( v.x, v.y, 0, 1 );
            #ifdef TEXTURED
              out_uv = in_uv;
            #endif
        })gl"};

    std::string fragment_shader{R"gl(
    #ifdef GL_ES
        precision mediump float;
    #endif
        uniform vec4 in_color;
        #ifdef TEXTURED
          uniform sampler2D in_tex;
          varying vec2 out_uv;
        #endif
        void main() {
            #ifdef TEXTURED
              gl_FragColor = texture2D(in_tex, out_uv) * in_color;
            #else
              gl_FragColor = in_color;
            #endif
        })gl"};

//#ifdef EMSCRIPTEN
//    static const inline std::string version = "#version 150 es\n";
//#else
    static const inline std::string version = "";
//#endif

    Program get_program(std::string_view prefix) const
    {
        using namespace std::string_literals;
        Shader<ShaderType::Vertex> vs{
            version + std::string(prefix) + vertex_shader};

        Shader<ShaderType::Fragment> fs{
            version + std::string(prefix) + fragment_shader};

        // Get info log
        auto info = getShaderInfoLog(vs.shader);
        fmt::print("{}\n", info);

        if (!fs || !vs) {
            fprintf(stderr, "Could not compile shaders\n");
            exit(0);
        }
        return Program(vs, fs);
    }

    ProgramCache()
    {
        non_textured = get_program("");
        textured = get_program("#define TEXTURED\n");
    }

    static ProgramCache& get_instance()
    {
        static ProgramCache pc;
        return pc;
    }
};

} // namespace gl_wrap
