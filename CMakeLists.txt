cmake_minimum_required(VERSION 3.10)
project(sdl_app)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)

include_directories(
    ./imgui
    ./nimble
    ${SDL2_INCLUDE_DIRS}
)

add_executable(sdl_app
    main.cpp
    nimble/utils/cursors.cpp
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/backends/imgui_impl_sdl2.cpp
    imgui/backends/imgui_impl_sdlrenderer2.cpp
)

target_link_libraries(sdl_app
    ${SDL2_LIBRARIES}
    SDL2_gfx
    SDL2_ttf
    SDL2_image
)