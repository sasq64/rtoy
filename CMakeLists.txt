cmake_minimum_required(VERSION 3.10)
project(rtoy VERSION 1.0)

include(ExternalProject)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
  add_compile_options(/utf-8)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES
                  -D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
  set(WARN_EXT /W2)
endif()

if(CMAKE_CXX_COMPILER MATCHES "/em\\+\\+(-[a-zA-Z0-9.])?$")
  message(" * Emscripten")
  set(CMAKE_CXX_COMPILER_ID "Emscripten")
  set(EMSCRIPTEN 1)
else()
  message(" * ${CMAKE_CXX_COMPILER_ID}")
endif()

find_package(Threads REQUIRED)

# Warning level setup #######################################################

# GCC/Clang common warnings
set(GCC_CLANG_WARNINGS
    -Werror
    -Wall
    -Wextra
    -Wnon-virtual-dtor
    -pedantic
    -Wno-unused-variable
    -Wno-unused-parameter
    -Wno-unused-function)

set(MSVC_WARNINGS /W2)

# GCC specific warnings
set(GCC_WARNINGS -Wduplicated-branches -Wduplicated-cond -Wlogical-op
                 -Wcast-align -Wno-missing-field-initializers)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(WARNINGS ${WARNINGS} ${GCC_WARNINGS} ${GCC_CLANG_WARNINGS})
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(WARNINGS ${WARNINGS} ${CLANG_WARNINGS} ${GCC_CLANG_WARNINGS})
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set(WARNINGS ${WARNINGS} ${MSVC_WARNINGS})
endif()

set(MRB ${PROJECT_SOURCE_DIR}/external/mruby)
set(MRB_LIB ${PROJECT_SOURCE_DIR}/builds/mruby/lib)
set(MRB_INC ${MRB}/include)
add_custom_command(
    OUTPUT ${MRB_LIB}/libmruby.a ${MRB_LIB}/libmruby_core.a
    COMMAND MRUBY_CONFIG=${PROJECT_SOURCE_DIR}/mruby.cfg rake -f ${MRB}/Rakefile -j8)

add_custom_target(mruby ALL
    DEPENDS ${MRB_LIB}/libmruby.a ${MRB_LIB}/libmruby_core.a mruby.cfg)


add_subdirectory(external/fmt)
add_subdirectory(external/CLI11)
add_subdirectory(external/coreutils)
add_subdirectory(external/glm)
#option(SKIP_PERFORMANCE_COMPARISON "" ON)
#add_subdirectory(external/doctest)
#add_subdirectory(external/cpp-httplib)

if(EMSCRIPTEN)
    add_compile_options(-sUSE_FREETYPE=1)
    add_compile_options(-sUSE_SDL=2)
    add_compile_options(-sFETCH=1)
    add_compile_options(-sERROR_ON_UNDEFINED_SYMBOLS=1)
else()
    find_package(SDL2 REQUIRED)
    include(FindFreetype)
    find_package(Freetype REQUIRED)
endif()


# Graphics
add_library(pix STATIC src/pix/pix.cpp src/pix/font.cpp src/pix/texture_font.cpp
    src/pix/font_renderer.cpp
    src/pix/gl_console.cpp external/lodepng/lodepng.cpp)
target_include_directories(
  pix
  PUBLIC ${SDL2_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS} /usr/local/include/SDL2
  PRIVATE external/lodepng)
target_include_directories(pix PUBLIC src)
#set(GL GLESv2)
set(GL GL GLEW)
target_link_libraries(pix PUBLIC SDL2 ${GL} Threads::Threads coreutils
                                 ${SDL2_LIBRARIES} freetype)

############## MAIN TARGET ###################################################

if(EMSCRIPTEN)
    set(TARGET toy.html)
    set(MRB_LIB ${PROJECT_SOURCE_DIR}/emhost/lib)
else()
    set(TARGET toy)
endif()

add_executable(${TARGET}
    src/main.cpp
    src/toy.cpp
    src/rlayer.cpp
    src/rdisplay.cpp
    src/rcanvas.cpp
    src/rconsole.cpp
    src/rimage.cpp
    src/rsprites.cpp
    src/rinput.cpp
    src/rtimer.cpp
    src/pix/gl_console.cpp)
target_include_directories(${TARGET} PUBLIC ${MRB_INC} external)
target_compile_definitions(${TARGET} PUBLIC -DMRB_INT32)
target_link_libraries(${TARGET} PRIVATE
    pix
    glm::glm
    Threads::Threads
    CLI11::CLI11
    ${MRB_LIB}/libmruby.a ${MRB_LIB}/libmruby_core.a
    ${CMAKE_DL_LIBS}
    fmt::fmt)

if(EMSCRIPTEN)
    
    target_link_options(toy.html PRIVATE "SHELL:-s MAX_WEBGL_VERSION=2")
    target_link_options(toy.html PRIVATE "SHELL:--preload-file ../../data@data")
    target_link_options(toy.html PRIVATE "SHELL:--preload-file ../../ruby@ruby")
    target_link_options(toy.html PRIVATE "-sFETCH=1")
else()
    #add_dependencies(toy mruby)
endif()
