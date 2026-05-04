echo g++ main.cpp \
sdl_ui_kit/utils/cursors.cpp \
imgui/imgui.cpp \
imgui/imgui_draw.cpp \
imgui/imgui_widgets.cpp \
imgui/imgui_tables.cpp \
imgui/backends/imgui_impl_sdl2.cpp \
imgui/backends/imgui_impl_sdlrenderer2.cpp \
-o sdl_app \
$(sdl2-config --cflags --libs) \
-lSDL2_gfx -lSDL2_ttf -lSDL2_image \
-I./imgui && 
g++ main.cpp \
sdl_ui_kit/utils/cursors.cpp \
imgui/imgui.cpp \
imgui/imgui_draw.cpp \
imgui/imgui_widgets.cpp \
imgui/imgui_tables.cpp \
imgui/backends/imgui_impl_sdl2.cpp \
imgui/backends/imgui_impl_sdlrenderer2.cpp \
-o sdl_app \
$(sdl2-config --cflags --libs) \
-lSDL2_gfx -lSDL2_ttf -lSDL2_image \
-I./imgui && ./sdl_app