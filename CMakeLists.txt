cmake_minimum_required(VERSION 3.20)

project(terebi LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

# Wayland Scanner for layer-shell
find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

# Dependencies
pkg_check_modules(WAYLAND REQUIRED IMPORTED_TARGET wayland-server)
pkg_check_modules(WLROOTS REQUIRED IMPORTED_TARGET wlroots-0.20)
pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)
pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)
pkg_check_modules(SDL3_IMAGE REQUIRED IMPORTED_TARGET sdl3-image)
pkg_check_modules(SDL3_TTF REQUIRED IMPORTED_TARGET sdl3-ttf)
pkg_check_modules(LIBDRM REQUIRED IMPORTED_TARGET libdrm)
pkg_check_modules(PIXMAN REQUIRED IMPORTED_TARGET pixman-1)

# Locate wlr-layer-shell protocol XML
find_file(LAYER_SHELL_XML
    NAMES wlr-layer-shell-unstable-v1.xml
    PATHS
        /usr/share/wlr-protocols/unstable
        /usr/local/share/wlr-protocols/unstable
        /usr/share/wayland-protocols/unstable/wlr-layer-shell
)

if(NOT LAYER_SHELL_XML)
    message(FATAL_ERROR "wlr-layer-shell-unstable-v1.xml not found!")
endif()

# Generate the protocol header
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-protocol.h"
    COMMAND ${WAYLAND_SCANNER} server-header ${LAYER_SHELL_XML} "${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-protocol.h"
    DEPENDS ${LAYER_SHELL_XML}
    COMMENT "Generating wlr-layer-shell-unstable-v1-protocol.h"
)

# ImGui Source files (SDL3)
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

add_executable(sdl_app
    main.cpp
    # compositor/wlroots.c
    terebi/remote_control.cpp
    nimble/utils/cursors.cpp
    ${IMGUI_SOURCES}
    "${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-protocol.h"
)

target_include_directories(sdl_app PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    # ${CMAKE_CURRENT_SOURCE_DIR}/compositor
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(sdl_app PRIVATE
    PkgConfig::WAYLAND
    PkgConfig::WLROOTS
    PkgConfig::XKBCOMMON
    PkgConfig::SDL3
    PkgConfig::SDL3_IMAGE
    PkgConfig::SDL3_TTF
    PkgConfig::LIBDRM
    PkgConfig::PIXMAN
    pthread
)

target_compile_options(sdl_app PRIVATE
    -DWLR_USE_UNSTABLE
)