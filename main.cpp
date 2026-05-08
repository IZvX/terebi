#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include "nimble/utils/sdl_compat.h"

// Include your UI Kit
#include "nimble/nimble.cpp"
#include "nimble/utils/cursors.h"
#include "fontawesome/fontawesome.h"

// ImGui
#include "imgui/backends/imgui_impl_sdl3.h"
#include "imgui/backends/imgui_impl_sdlrenderer3.h"
#include "imgui/imgui.h"

// --- Include your new separated files ---
#include "components/shared.h"
#include "components/inspector.h"
#include "screens/home_page.h"
#include "utils/theme.h"

// Forward Declarations
void initUIKit();
// ADDED: bool &stateChanged to track if we need to wake up
InputState gatherInputState(SDL_Event &e, bool &quit, bool &stateChanged);
InputState gatherInputState(SDL_Event &e, bool &quit, bool &stateChanged, int waitTimeoutMs);

static bool HasRendererDriver(const char *name)
{
    const int numDrivers = SDL_GetNumRenderDrivers();
    for (int i = 0; i < numDrivers; ++i)
    {
        const char *driver = SDL_GetRenderDriver(i);
        if (driver && SDL_strcasecmp(driver, name) == 0)
            return true;
    }
    return false;
}

static SDL_Renderer *CreateBestRenderer(SDL_Window *window, const std::string &preferredRenderer)
{
    // SDL2 -> SDL3 migration: renderer creation now accepts a driver name.
    // Prefer Vulkan when supported; otherwise pick the next available GPU backend.
    std::vector<const char *> preferredDrivers = {
        "vulkan",
        "metal",
        "direct3d12",
        "direct3d11",
        "opengl",
        "opengles2"
    };

    if (!preferredRenderer.empty())
    {
        preferredDrivers.erase(
            std::remove_if(
                preferredDrivers.begin(),
                preferredDrivers.end(),
                [&](const char *driver) { return SDL_strcasecmp(driver, preferredRenderer.c_str()) == 0; }),
            preferredDrivers.end());
        preferredDrivers.insert(preferredDrivers.begin(), preferredRenderer.c_str());
    }

    std::unordered_set<std::string> triedDrivers;
    auto tryDriver = [&](const char *driverName) -> SDL_Renderer * {
        if (!driverName) return nullptr;
        std::string key = driverName;
        if (triedDrivers.count(key)) return nullptr;
        triedDrivers.insert(key);

        SDL_Renderer *candidate = SDL_CreateRenderer(window, driverName);
        if (!candidate) return nullptr;

        const char *active = SDL_GetRendererName(candidate);
        if (active && SDL_strcasecmp(active, "software") == 0)
        {
            SDL_DestroyRenderer(candidate);
            return nullptr;
        }
        return candidate;
    };

    for (const char *driver : preferredDrivers)
    {
        if (!HasRendererDriver(driver))
            continue;
        if (SDL_Renderer *renderer = tryDriver(driver))
            return renderer;
    }

    // Dynamic fallback: try every available non-software backend.
    const int numDrivers = SDL_GetNumRenderDrivers();
    for (int i = 0; i < numDrivers; ++i)
    {
        const char *driver = SDL_GetRenderDriver(i);
        if (SDL_Renderer *renderer = tryDriver(driver))
            return renderer;
    }

    // Last resort.
    return SDL_CreateRenderer(window, "software");
}

int main(int argc, char *args[])
{
    std::string preferredRenderer;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = args[i];
        if ((arg == "-r" || arg == "--renderer") && (i + 1) < argc)
        {
            preferredRenderer = args[++i];
        }
    }

    // ====================== INITIALIZATION ======================
    if (!SDL_Init(SDL_INIT_VIDEO)) return -1;
    if (TTF_Init() == -1) return -1;
    IMG_Init(IMG_INIT_PNG);

    // SDL2 -> SDL3 migration: present-vsync is now controlled with SDL_HINT_RENDER_VSYNC.
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    // SDL2 -> SDL3 migration: SDL_CreateWindow() no longer accepts x/y position parameters.
    SDL_Window *window = SDL_CreateWindow("Terebi UI",
                                          800, 600,
                                          SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

    SDL_Renderer *renderer = CreateBestRenderer(window, preferredRenderer);
    if (!renderer) return -1;

    // ====================== ImGui INIT ======================
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // SDL2 -> SDL3 migration: backend names changed from SDL2/SDLRenderer2 to SDL3/SDLRenderer3.
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    // ===================== FONT LOADING =====================
    auto loadFont =[](const std::string &path, int size) -> TTF_Font * {
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
    // SDL2 -> SDL3 migration: timer APIs in SDL3 use 64-bit millisecond values.
    Uint64 lastTime = SDL_GetTicks();

    // --- Render scheduling / performance variables ---
    int awakeFrames = 120; // retained for debug/UI signaling
    Uint64 lastDrawTime = SDL_GetTicks();

    GlobalContext gctx = {
        .settingsOpen = true
    };

    // ====================== MAIN LOOP =======================
    while (!quit)
    {
        bool stateChanged = false;
        const bool likelyIdle = (awakeFrames <= 0) && !HasPendingVisualUpdates();
        InputState input = gatherInputState(e, quit, stateChanged, likelyIdle ? 16 : 0);

        // Keep wake signal for debug/UI instrumentation.
        if (stateChanged) {
            awakeFrames = 120;
        }

        Uint64 currentTime = SDL_GetTicks();
        const bool pendingVisualUpdates = HasPendingVisualUpdates();
        const bool shouldRender = stateChanged || pendingVisualUpdates || (awakeFrames > 0);

        if (!shouldRender) {
            continue;
        }

        // Decrease awake counter if we are active
        if (awakeFrames > 0) {
            awakeFrames--;
        }

        // Calculate DeltaTime only for frames we actually render.
        float dt = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
        lastDrawTime = currentTime;

        // Start Frames
        ImGui_ImplSDL3_NewFrame();
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui::NewFrame();

        // UI Kit Frame Management
        StartUIFrame();
        UpdateUIAnimations(dt);
        ResetCursor();

        int ww, wh;
        SDL_GetWindowSize(window, &ww, &wh);

        // ====================== RENDER SCREEN ======================
        // We call our separated screen here
        Widget screen = HomePage(ww, wh, dt, searchString, fonts, gctx);

        SDL_SetRenderDrawColor(renderer, 30, 30, 35, 255);
        SDL_RenderClear(renderer);

        screen.render(renderer, {0, 0, ww, wh}, input);

        // ====================== DEBUG WINDOW ======================
        ImGui::Begin("Debug Info");
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Performance: %s", pendingVisualUpdates ? "Animated" : "Idle/event-driven");

        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Mouse: %d, %d", input.mouseX, input.mouseY);
        ImGui::Text("Renderer: %s", SDL_GetRendererName(renderer));
        ImGui::Text("Focused: %s", g_FocusedWidgetId.c_str());
        ImGui::Text("Next Focus: %s", g_NextFocusedWidgetId.c_str());

        ImGui::Separator();

        if (ImGui::CollapsingHeader("Global Settings")) {
            ImGui::Checkbox("Show Widget IDs", &g_Settings.showWidgetIds);
            ImGui::Checkbox("Show Clip Rects", &g_Settings.showClipRects);
            ImGui::Checkbox("Show Flex Weights", &g_Settings.showFlexWeights);
            ImGui::Checkbox("Show Focus Loop", &g_Settings.showFocusLoop);
            ImGui::Checkbox("Highlight Input Capture", &g_Settings.highlightInputCapture);
            ImGui::Checkbox("Slow Animations", &g_Settings.slowAnimations);
            ImGui::Checkbox("Paint Flash Mode", &g_Settings.paintFlashMode);
            ImGui::Checkbox("Show FPS Overlay", &g_Settings.showFPSOverlay);
            ImGui::Checkbox("Trigger State Reset", &g_Settings.triggerStateReset);
        }

        if (ImGui::CollapsingHeader("Debug Draw")) {
            ImGui::Checkbox("Enable Debug",    &g_GlobalDebug.enabled        );
            ImGui::Checkbox("Show Bounds",     &g_GlobalDebug.showBounds     );
            ImGui::Checkbox("Show Padding",    &g_GlobalDebug.showPadding    );
            ImGui::Checkbox("Show Spacing",    &g_GlobalDebug.showSpacing    );
            ImGui::Checkbox("Show Expanded",   &g_GlobalDebug.showExpanded   );
            ImGui::Checkbox("Show Row",        &g_GlobalDebug.showRow        );
            ImGui::Checkbox("Show Column",     &g_GlobalDebug.showColumn     );
            ImGui::Checkbox("Show Nav Arrows", &g_GlobalDebug.showNavArrows  );
        }

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
                awakeFrames = 120; // Manually wake up for the ImGui button press!
            }
            ImGui::SameLine();
            if (ImGui::Button("Collapse SearchButton"))
            {
                g_SearchState["navbar_search"].expanded = false;
                g_SearchState["navbar_search"].focusSent = false;
                g_NextFocusedWidgetId = "";
                awakeFrames = 120;
            }

        }

        if (ImGui::CollapsingHeader("Settings Drawer Debug")) {
            if (ImGui::Button("Toggle Settings Drawer")) {
                g_Context.settingsOpen = !g_Context.settingsOpen;
                awakeFrames = 120;
            }
        }

        if (ImGui::CollapsingHeader("Pywal Theme"))
        {
            if (ImGui::Checkbox("Enable Pywal", &g_Context.pywalEnabled)) awakeFrames = 120;

            if (ImGui::Button("Reload Pywal Theme"))
            {
                g_Context.currentTheme = WalLoadTheme();
                awakeFrames = 120;
            }

            ImGui::Separator();

            WalTheme &wal = g_Context.currentTheme;

            auto DrawColor =[](const char* name, SDL_Color col)
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

        // ... (Remaining color pickers) ...

        ImGui::End();
        // Render ImGui
        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

        SDL_RenderPresent(renderer);
    }

    // ====================== CLEANUP ======================
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
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

// UPDATED: Added bool &stateChanged flag
InputState gatherInputState(SDL_Event &e, bool &quit, bool &stateChanged)
{
    return gatherInputState(e, quit, stateChanged, 0);
}

InputState gatherInputState(SDL_Event &e, bool &quit, bool &stateChanged, int waitTimeoutMs)
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

    auto processEvent = [&](SDL_Event &ev)
    {
        // Any SDL event happening (resize, key, click, hover ImGui) wakes up the app!
        stateChanged = true;

        ImGui_ImplSDL3_ProcessEvent(&ev);

        if (ev.type == SDL_EVENT_QUIT)
            quit = true;
        else if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            if (ev.button.button == SDL_BUTTON_LEFT)
                input.mouseClicked = true;
            else if (ev.button.button == SDL_BUTTON_RIGHT)
                input.rightMouseClicked = true;
        }
        else if (ev.type == SDL_EVENT_MOUSE_WHEEL)
        {
            input.mouseWheelX = (float)ev.wheel.x;
            input.mouseWheelY = (float)ev.wheel.y;
        }
        else if (ev.type == SDL_EVENT_KEY_DOWN)
        {
            // SDL2 -> SDL3 migration: keyboard event key/mod moved from keysym to key/mod fields.
            input.keyPressed = ev.key.key;
            input.keyMod = ev.key.mod;
            input.backspacePressed = (ev.key.key == SDLK_BACKSPACE);
            input.deletePressed = (ev.key.key == SDLK_DELETE);
            input.leftPressed = (ev.key.key == SDLK_LEFT);
            input.rightPressed = (ev.key.key == SDLK_RIGHT);
            input.enterPressed =
                (ev.key.key == SDLK_RETURN || ev.key.key == SDLK_KP_ENTER);
        }
        else if (ev.type == SDL_EVENT_TEXT_INPUT)
        {
            input.textInput += ev.text.text;
        }
    };

    if (waitTimeoutMs > 0)
    {
        if (SDL_WaitEventTimeout(&e, waitTimeoutMs))
            processEvent(e);
    }

    while (SDL_PollEvent(&e))
    {
        processEvent(e);
    }

    float mouseX = 0.0f;
    float mouseY = 0.0f;
    Uint32 buttons = SDL_GetMouseState(&mouseX, &mouseY);
    input.mouseX = (int)mouseX;
    input.mouseY = (int)mouseY;
    input.leftMouseDown = (buttons & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0;
    input.rightMouseDown = (buttons & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT)) != 0;

    return input;
}