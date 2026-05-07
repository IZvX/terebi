#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <string>

// Include your UI Kit
#include "nimble/nimble.cpp"
#include "nimble/utils/cursors.h"
#include "fontawesome/fontawesome.h"

// ImGui
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_sdlrenderer2.h"
#include "imgui/imgui.h"

// --- Include your new separated files ---
#include "components/shared.h"
#include "components/inspector.h"
#include "screens/home_page.h"
#include "utils/theme.h"

// Forward Declarations
void initUIKit();
InputState gatherInputState(SDL_Event &e, bool &quit);

int main(int argc, char *args[])
{
    // ====================== INITIALIZATION ======================
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return -1;
    if (TTF_Init() == -1) return -1;
    IMG_Init(IMG_INIT_PNG);

    SDL_Window *window = SDL_CreateWindow("Terebi UI",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          800, 600,
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // ====================== ImGui INIT ======================
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // ===================== FONT LOADING =====================
    auto loadFont = [](const std::string &path, int size) -> TTF_Font * {
        TTF_Font *font = TTF_OpenFont(path.c_str(), size);
        if (!font) std::cout << "Warning: Failed to load: " << path << std::endl;
        return font;
    };

    AppFonts fonts;
    fonts.spaceGrotesk12 = loadFont("assets/fonts/spacegrotesk/SpaceGrotesk-700.ttf", 12);
    fonts.spaceGrotesk24 = loadFont("assets/fonts/spacegrotesk/SpaceGrotesk-700.ttf", 24);
    fonts.spaceGrotesk48 = loadFont("assets/fonts/spacegrotesk/SpaceGrotesk-700.ttf", 48);
    fonts.arial12       = loadFont("assets/fonts/Arial.ttf", 12);
    fonts.arial14       = loadFont("assets/fonts/Arial.ttf", 14);
    fonts.arial16       = loadFont("assets/fonts/Arial.ttf", 16);
    fonts.arial18       = loadFont("assets/fonts/Arial.ttf", 18);
    fonts.arial20       = loadFont("assets/fonts/Arial.ttf", 20);
    fonts.arial28       = loadFont("assets/fonts/Arial.ttf", 28);
    fonts.fontAwesome24 = loadFont("fontawesome/fa-solid-900.otf", 24);
    fonts.fontAwesomeB24 = loadFont("fontawesome/fa-brands-400.otf", 24);

    // ====================== APP STATE =======================
    initUIKit();
    Cursors_Init();
    g_NextFocusedWidgetId = "navbar_home"; // Default focus
    std::string searchString = "";
    g_Context.wifiToggled = (bool)nmcli::enabled;

    // Pre-load pywal theme (but don't enable it - user controls that with the toggle)
    g_Context.currentTheme = WalLoadTheme();
    g_Context.pywalEnabled = false; // Start with pywal disabled

    bool quit = false;
    SDL_Event e;
    Uint32 lastTime = SDL_GetTicks();

    GlobalContext gctx = {
        .settingsOpen = true
    };

    // ====================== MAIN LOOP =======================
    while (!quit)
    {
        InputState input = gatherInputState(e, quit);

        Uint32 currentTime = SDL_GetTicks();
        float dt = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // Start Frames
        ImGui_ImplSDL2_NewFrame();
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui::NewFrame();

        // UI Kit Frame Management
        StartUIFrame();
        UpdateUIAnimations(dt);
        ResetCursor();

        int ww, wh;
        SDL_GetWindowSize(window, &ww, &wh);

        // ====================== RENDER SCREEN ======================
        // We call our separated screen here
        Widget screen = HomePage(ww, wh, dt, searchString, fonts,gctx);

        SDL_SetRenderDrawColor(renderer, 30, 30, 35, 255);
        SDL_RenderClear(renderer);

        screen.render(renderer, {0, 0, ww, wh}, input);

        // ====================== DEBUG WINDOW ======================
        ImGui::Begin("Debug Info");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Mouse: %d, %d", input.mouseX, input.mouseY);
        ImGui::Text("Focused: %s", g_FocusedWidgetId.c_str());
        ImGui::Text("Next Focus: %s", g_NextFocusedWidgetId.c_str());

        ImGui::Text("FocusedId:     %s", g_FocusedWidgetId.c_str());
        ImGui::Text("NextFocusedId: %s", g_NextFocusedWidgetId.c_str());

        ImGui::Separator();

        ImGui::Checkbox("Enable Debug", &g_GlobalDebug.enabled);
        ImGui::Checkbox("Show Bounds", &g_GlobalDebug.showBounds);
        ImGui::Checkbox("Show Padding", &g_GlobalDebug.showPadding);
        ImGui::Checkbox("Show Spacing", &g_GlobalDebug.showSpacing);
        ImGui::Checkbox("Show Expanded", &g_GlobalDebug.showExpanded);
        ImGui::Checkbox("Show Row", &g_GlobalDebug.showRow);
        ImGui::Checkbox("Show Column", &g_GlobalDebug.showColumn);
        ImGui::Checkbox("Show Nav Arrows", &g_GlobalDebug.showNavArrows);

        ImGui::Separator();
        if (ImGui::CollapsingHeader("Search Button Debug")) {

            ImGui::Text("SearchButton State");
            SearchButtonState &dbgSS = g_SearchState["navbar_search"];
            ImGui::Text("expanded:  %s", dbgSS.expanded ? "true" : "false");
            ImGui::Text("t:         %.3f", dbgSS.t);
            ImGui::Text("focusSent: %s", dbgSS.focusSent ? "true" : "false");
            ImGui::Text("isActive:  %s",
                        (g_FocusedWidgetId == "navbar_search" ||
                         g_FocusedWidgetId == "nav_searchbar_text" ||
                         g_NextFocusedWidgetId == "navbar_search" ||
                         g_NextFocusedWidgetId == "nav_searchbar_text")
                        ? "true"
                        : "false");
            if (ImGui::Button("Expand SearchButton"))
            {
                g_SearchState["navbar_search"].expanded = true;
                g_SearchState["navbar_search"].focusSent = false;
                g_NextFocusedWidgetId = "navbar_search";
            }
            ImGui::SameLine();
            if (ImGui::Button("Collapse SearchButton"))
            {
                g_SearchState["navbar_search"].expanded = false;
                g_SearchState["navbar_search"].focusSent = false;
                g_NextFocusedWidgetId = "";
            }

        }

        if (ImGui::CollapsingHeader("Settings Drawer Debug")) {
            if (ImGui::Button("Toggle Settings Drawer")) {
                g_Context.settingsOpen = !g_Context.settingsOpen;
            }
        }

        if (ImGui::CollapsingHeader("Pywal Theme"))
        {
            ImGui::Checkbox("Enable Pywal", &g_Context.pywalEnabled);

            if (ImGui::Button("Reload Pywal Theme"))
            {
                g_Context.currentTheme = WalLoadTheme();
            }

            ImGui::Separator();

            WalTheme &wal = g_Context.currentTheme;

            auto DrawColor = [](const char* name, SDL_Color col)
            {
                float color[4] = {
                    col.r / 255.0f,
                    col.g / 255.0f,
                    col.b / 255.0f,
                    col.a / 255.0f
                };

                ImGui::ColorEdit4(
                    name,
                    color,
                    ImGuiColorEditFlags_NoInputs |
                    ImGuiColorEditFlags_AlphaPreview |
                    ImGuiColorEditFlags_AlphaBar
                );

                ImGui::SameLine();

                ImGui::Text(
                    "#%02X%02X%02X",
                    col.r,
                    col.g,
                    col.b
                );
            };

            DrawColor("background", wal.background);
            DrawColor("foreground", wal.foreground);
            DrawColor("cursor", wal.cursor);

            ImGui::Separator();

            DrawColor("color0", wal.color0);
            DrawColor("color1", wal.color1);
            DrawColor("color2", wal.color2);
            DrawColor("color3", wal.color3);
            DrawColor("color4", wal.color4);
            DrawColor("color5", wal.color5);
            DrawColor("color6", wal.color6);
            DrawColor("color7", wal.color7);

            ImGui::Separator();

            DrawColor("color8", wal.color8);
            DrawColor("color9", wal.color9);
            DrawColor("color10", wal.color10);
            DrawColor("color11", wal.color11);
            DrawColor("color12", wal.color12);
            DrawColor("color13", wal.color13);
            DrawColor("color14", wal.color14);
            DrawColor("color15", wal.color15);
        }
        ImGui::Separator();
        ImGui::Text("Colors:");

        // Color controls
        if (g_GlobalDebug.showBounds)
        {
            float boundsCol[4] = {g_GlobalDebug.boundsColor.r / 255.0f,
                                  g_GlobalDebug.boundsColor.g / 255.0f,
                                  g_GlobalDebug.boundsColor.b / 255.0f,
                                  g_GlobalDebug.boundsColor.a / 255.0f
                                 };
            if (ImGui::ColorEdit4("Bounds Color", boundsCol, ImGuiColorEditFlags_AlphaBar))
            {
                g_GlobalDebug.boundsColor.r = (Uint8)(boundsCol[0] * 255.0f);
                g_GlobalDebug.boundsColor.g = (Uint8)(boundsCol[1] * 255.0f);
                g_GlobalDebug.boundsColor.b = (Uint8)(boundsCol[2] * 255.0f);
                g_GlobalDebug.boundsColor.a = (Uint8)(boundsCol[3] * 255.0f);
            }
        }

        if (g_GlobalDebug.showPadding)
        {
            float paddingCol[4] = {g_GlobalDebug.paddingColor.r / 255.0f,
                                   g_GlobalDebug.paddingColor.g / 255.0f,
                                   g_GlobalDebug.paddingColor.b / 255.0f,
                                   g_GlobalDebug.paddingColor.a / 255.0f
                                  };
            if (ImGui::ColorEdit4("Padding Color", paddingCol, ImGuiColorEditFlags_AlphaBar))
            {
                g_GlobalDebug.paddingColor.r = (Uint8)(paddingCol[0] * 255.0f);
                g_GlobalDebug.paddingColor.g = (Uint8)(paddingCol[1] * 255.0f);
                g_GlobalDebug.paddingColor.b = (Uint8)(paddingCol[2] * 255.0f);
                g_GlobalDebug.paddingColor.a = (Uint8)(paddingCol[3] * 255.0f);
            }
        }

        if (g_GlobalDebug.showSpacing)
        {
            float spacingCol[4] = {g_GlobalDebug.spacingColor.r / 255.0f,
                                   g_GlobalDebug.spacingColor.g / 255.0f,
                                   g_GlobalDebug.spacingColor.b / 255.0f,
                                   g_GlobalDebug.spacingColor.a / 255.0f
                                  };
            if (ImGui::ColorEdit4("Spacing Color", spacingCol, ImGuiColorEditFlags_AlphaBar))
            {
                g_GlobalDebug.spacingColor.r = (Uint8)(spacingCol[0] * 255.0f);
                g_GlobalDebug.spacingColor.g = (Uint8)(spacingCol[1] * 255.0f);
                g_GlobalDebug.spacingColor.b = (Uint8)(spacingCol[2] * 255.0f);
                g_GlobalDebug.spacingColor.a = (Uint8)(spacingCol[3] * 255.0f);
            }
        }
        ImGui::End();
        // Render ImGui
        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

        SDL_RenderPresent(renderer);
    }

    // ====================== CLEANUP ======================
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    Cursors_Quit();

    return 0;
}

void initUIKit() {
    std::cout << "[UI Kit] Initialized." << std::endl;
}

InputState gatherInputState(SDL_Event &e, bool &quit)
{
    InputState input = {};
    input.textInput = "";
    input.backspacePressed = false;
    input.deletePressed = false;
    input.leftPressed = false;
    input.rightPressed = false;
    input.enterPressed = false;
    input.mouseClicked = false;
    input.rightMouseClicked = false;
    input.mouseWheelX = 0.0f;
    input.mouseWheelY = 0.0f;
    input.leftMouseDown = false;
    input.rightMouseDown = false;
    input.keyPressed = SDLK_UNKNOWN;
    input.keyMod = 0;

    while (SDL_PollEvent(&e))
    {
        ImGui_ImplSDL2_ProcessEvent(&e);

        if (e.type == SDL_QUIT)
            quit = true;
        else if (e.type == SDL_MOUSEBUTTONDOWN)
        {
            if (e.button.button == SDL_BUTTON_LEFT)
                input.mouseClicked = true;
            else if (e.button.button == SDL_BUTTON_RIGHT)
                input.rightMouseClicked = true;
        }
        else if (e.type == SDL_MOUSEWHEEL)
        {
            input.mouseWheelX = (float)e.wheel.x;
            input.mouseWheelY = (float)e.wheel.y;
        }
        else if (e.type == SDL_KEYDOWN)
        {
            input.keyPressed = e.key.keysym.sym;
            input.keyMod = e.key.keysym.mod;
            input.backspacePressed = (e.key.keysym.sym == SDLK_BACKSPACE);
            input.deletePressed = (e.key.keysym.sym == SDLK_DELETE);
            input.leftPressed = (e.key.keysym.sym == SDLK_LEFT);
            input.rightPressed = (e.key.keysym.sym == SDLK_RIGHT);
            input.enterPressed =
                (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER);
        }
        else if (e.type == SDL_TEXTINPUT)
        {
            input.textInput += e.text.text;
        }
    }

    Uint32 buttons = SDL_GetMouseState(&input.mouseX, &input.mouseY);
    input.leftMouseDown = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    input.rightMouseDown = (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
    return input;
}