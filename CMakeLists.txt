cmake_minimum_required(VERSION 3.10)
project(rtoy VERSION 1.0)

include(ExternalProject)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

cmake_policy(SET CMP0072 NEW)

if(EXISTS /opt/vc)
    set(RASPBERRY_PI 1)
endif()

if(RASPBERRY_PI)
    set(VC /opt/vc)
    include_directories(
        ${VC}/include
        ${VC}/include/interface/vcos/pthreads
        ${VC}/include/interface/vmcs_host/linux)
    link_directories(${VC}/lib)
    add_compile_definitions(RASPBERRY_PI)
endif()

if(WIN32)
  add_compile_options(/utf-8)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES
                  -D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
  set(WARN_EXT /W2)
endif()

if(APPLE)
    include_directories(/usr/local/include)
    link_directories(/usr/local/lib)
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

# add_custom_command(
#  OUTPUT ${MRB_LIB}/libmruby.a ${MRB_LIB}/libmruby_core.a
#  COMMAND
#  MRUBY_CONFIG=${PROJECT_SOURCE_DIR}/mruby.cfg rake -f ${MRB}/Rakefile -j8)
# add_custom_target(mruby ALL
#     DEPENDS ${MRB_LIB}/libmruby.a ${MRB_LIB}/libmruby_core.a mruby.cfg)

add_subdirectory(external/fmt)
add_subdirectory(external/CLI11)
add_subdirectory(external/coreutils)
add_subdirectory(external/glm)
add_subdirectory(external/doctest)
#option(SKIP_PERFORMANCE_COMPARISON "" ON)
#add_subdirectory(external/cpp-httplib)

if(RASPBERRY_PI)
    include(FindFreetype)
    find_package(Freetype REQUIRED)
    find_package(OpenGL)
    set(GL ${OPENGL_LIBRARIES} stdc++fs EGL GLESv2 vcos vchiq_arm bcm_host)
    add_compile_definitions(USE_GLES)
elseif(NOT EMSCRIPTEN)
    find_package(SDL2 REQUIRED)
    include(FindFreetype)
    find_package(Freetype REQUIRED)
    find_package(OpenGL)
    set(GL ${OPENGL_LIBRARIES} GLEW)
endif()


# set(MUS_LIB ${PROJECT_SOURCE_DIR}/../musicplayer)
# add_library(music INTERFACE)
# target_link_libraries(music INTERFACE ${MUS_LIB}/build/libmusix.a)
# target_include_directories(music INTERFACE ${MUS_LIB})

# Graphics
add_library(pix STATIC
    src/pix/pix.cpp
    src/pix/font.cpp
    src/pix/texture_font.cpp
    src/pix/tile_renderer.cpp
    src/pix/gl_console.cpp
    external/lodepng/lodepng.cpp)

if(EMSCRIPTEN)
    target_compile_options(pix PUBLIC -sUSE_FREETYPE=1)
    target_compile_options(pix PUBLIC -sUSE_SDL=2)
endif()

target_include_directories(pix
  PUBLIC ${SDL2_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS} /usr/local/include/SDL2
  PRIVATE external/lodepng)
target_include_directories(pix PUBLIC src)

target_link_libraries( pix PUBLIC
    ${GL} Threads::Threads coreutils ${SDL2_LIBRARIES} freetype)

############## MAIN TARGET ###################################################

if(EMSCRIPTEN)
    set(TARGET toy.html)
    set(MRB_LIB ${PROJECT_SOURCE_DIR}/builds/embuild/lib)
    set(FLITE_LIB
        ${PROJECT_SOURCE_DIR}/external/flite/build/x86_64-emscripten)
else()
    set(TARGET toy)
    set(FLITE_LIB
        ${PROJECT_SOURCE_DIR}/external/flite/build/x86_64-linux-gnu)
endif()

set(SRC_FILES
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
    src/rfont.cpp
    src/raudio.cpp
    src/rspeech.cpp
    src/pix/gl_console.cpp)

if(RASPBERRY_PI)
    set(SRC_FILES ${SRC_FILES} src/pi_system.cpp src/eglutil.cpp)
else()
    set(SRC_FILES ${SRC_FILES} src/sdl_system.cpp)
endif()

add_executable(${TARGET} ${SRC_FILES})

target_include_directories(${TARGET} PUBLIC
    ${MRB_INC} external external/flite/include)
target_compile_definitions(${TARGET} PUBLIC -DMRB_INT32)
target_link_options(${TARGET} PRIVATE -L${FLITE_LIB}/lib)
target_link_libraries(${TARGET} PRIVATE
    pix
    #music
    flite_cmu_us_kal
    flite_usenglish
    flite_cmulex
    flite
    glm::glm
    Threads::Threads
    CLI11::CLI11
    ${MRB_LIB}/libmruby.a ${MRB_LIB}/libmruby_core.a
    ${CMAKE_DL_LIBS}
    fmt::fmt)


if(NOT EMSCRIPTEN)
    add_executable(con contest.cpp src/sdl_system.cpp)
    target_link_libraries(con PRIVATE pix fmt::fmt)
endif()

#add_executable(toytest src/tests/testmain.cpp src/tests/resample_test.cpp)
#target_link_libraries(toytest PRIVATE fmt::fmt doctest::doctest resample)

if(EMSCRIPTEN)
    target_link_options(toy.html PRIVATE "SHELL:-s MAX_WEBGL_VERSION=2")
    target_link_options(toy.html PRIVATE "SHELL:-s MIN_WEBGL_VERSION=2")
    target_link_options(toy.html PRIVATE
        "SHELL:--preload-file ../../data@data")
    target_link_options(toy.html PRIVATE
        "SHELL:--preload-file ../../ruby@ruby")
    target_link_options(toy.html PRIVATE
        "SHELL:--preload-file ../../games@games")
    target_link_options(toy.html PRIVATE "-sUSE_SDL=2")
    target_link_options(toy.html PRIVATE "-sFETCH=1")
    target_link_options(toy.html PRIVATE -sERROR_ON_UNDEFINED_SYMBOLS=1)
endif()

