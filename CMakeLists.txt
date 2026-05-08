cmake_minimum_required(VERSION 3.10)
project(sdl_app)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_image REQUIRED CONFIG)
find_package(SDL3_ttf REQUIRED CONFIG)

include_directories(
    ./imgui
    ./nimble
)

add_executable(sdl_app
    main.cpp
    nimble/utils/cursors.cpp
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/backends/imgui_impl_sdl3.cpp
    imgui/backends/imgui_impl_sdlrenderer3.cpp
)

target_compile_definitions(sdl_app PRIVATE SDL_ENABLE_OLD_NAMES)

target_link_libraries(sdl_app
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf
)