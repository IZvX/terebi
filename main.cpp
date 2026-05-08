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

// GPU Renderer
#include "nimble/gpu/renderer_gpu.h"

// ImGui
#include "imgui/backends/imgui_impl_sdl3.h"
// Switch to the SDL_GPU3 backend
#include "imgui/backends/imgui_impl_sdlgpu3.h"
#include "imgui/imgui.h"

// --- Include your new separated files ---
#include "components/shared.h"
#include "components/inspector.h"
#include "screens/home_page.h"
#include "utils/theme.h"

// Forward Declarations
void initUIKit();
InputState gatherInputState(SDL_Event &e, bool &quit, bool &stateChanged);
InputState gatherInputState(SDL_Event &e, bool &quit, bool &stateChanged, int waitTimeoutMs);

int main(int argc, char *args[])
{
    // ====================== INITIALIZATION ======================
    if (!SDL_Init(SDL_INIT_VIDEO)) return -1;
    if (TTF_Init() == -1) return -1;
    IMG_Init(IMG_INIT_PNG);

    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    SDL_Window *window = SDL_CreateWindow("Terebi UI", 800, 600, SDL_WINDOW_RESIZABLE);

    // Initialize GPU Renderer exclusively
    GPURenderer* gpuRenderer = GetGPURenderer();
    if (!gpuRenderer->Initialize(window)) {
        std::cerr << "Failed to initialize GPU renderer! Exiting..." << std::endl;
        return -1;
    }
    
    // Load GPU shaders
    std::cout << "[GPU] Loading shaders..." << std::endl;
    
    // IMPORTANT: Make sure this file is named default.vert and compiled to .spv!
    std::string vertShader = "nimble/shaders/spirv/default.vert.spv";

    gpuRenderer->LoadShader(ShaderType::RoundedBox, vertShader, "nimble/shaders/spirv/rounded_box.frag.spv");
    gpuRenderer->LoadShader(ShaderType::BoxShadow,  vertShader, "nimble/shaders/spirv/box_shadow.frag.spv");
    gpuRenderer->LoadShader(ShaderType::Blur,       vertShader, "nimble/shaders/spirv/blur.frag.spv");
    
    std::cout << "[GPU] Shaders loaded successfully" << std::endl;
    
    // ====================== ImGui INIT ======================
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    ImGui_ImplSDL3_InitForSDLGPU(window);

    // Initialize ImGui for SDL_GPU
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = gpuRenderer->GetDevice();
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpuRenderer->GetDevice(), window);
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    ImGui_ImplSDLGPU3_Init(&init_info);

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

    g_Context.currentTheme = WalLoadTheme();
    g_Context.pywalEnabled = false;

    bool quit = false;
    SDL_Event e;
    Uint64 lastTime = SDL_GetTicks();
    int awakeFrames = 120; 
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

        if (stateChanged) {
            awakeFrames = 120;
        }

        Uint64 currentTime = SDL_GetTicks();
        const bool pendingVisualUpdates = HasPendingVisualUpdates();
        const bool shouldRender = stateChanged || pendingVisualUpdates || (awakeFrames > 0);

        if (!shouldRender) continue;

        if (awakeFrames > 0) awakeFrames--;

        float dt = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
        lastDrawTime = currentTime;

        // Start Frames
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        StartUIFrame();
        UpdateUIAnimations(dt);
        ResetCursor();

        int ww, wh;
        SDL_GetWindowSize(window, &ww, &wh);

        // ====================== RENDER SCREEN ======================
        Widget screen = HomePage(ww, wh, dt, searchString, fonts, gctx);

        // Set clear color and begin the GPU frame
        gpuRenderer->Clear(30.0f / 255.0f, 30.0f / 255.0f, 35.0f / 255.0f, 1.0f);
        if (gpuRenderer->AcquireCommandBuffer()) 
        {
            gpuRenderer->BeginFrame();

            // Note: We are passing nullptr here because we removed SDL_Renderer.
            // If Widget::render() internally requires an SDL_Renderer*, you will need 
            // to update your UI kit to not use it anymore!
            // screen.render(nullptr, {0, 0, ww, wh}, input);

            // ====================== DEBUG WINDOW ======================
            ImGui::Begin("Debug Info");
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Performance: %s", pendingVisualUpdates ? "Animated" : "Idle/event-driven");
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::Text("Mouse: %d, %d", input.mouseX, input.mouseY);
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

            ImGui::End();

            // Render ImGui onto the GPU Command Buffer
            ImGui::Render();
            ImGui_ImplSDLGPU3_RenderDrawData(ImGui::GetDrawData(), gpuRenderer->GetCommandBuffer(), gpuRenderer->GetRenderPass());

            gpuRenderer->EndFrame();
        }
    }

    // ====================== CLEANUP ======================
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    gpuRenderer->Shutdown();
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