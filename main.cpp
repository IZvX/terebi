#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>

// Include your UI Kit
#include "sdl_ui_kit/sdl_ui_kit.cpp"
#include "sdl_ui_kit/utils/cursors.h"

// Font Awesome
#include "fontawesome/fontawesome.h"

// ImGui
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_sdlrenderer2.h"
#include "imgui/imgui.h"

// =============================================
// Forward Declarations
// =============================================
void initUIKit();
InputState gatherInputState(SDL_Event &e, bool &quit);

#include <unordered_map>
#include <string>

// --- 1. The Override State ---
// This stores our real-time tweaks so they survive the frame-by-frame recreation
struct WidgetOverrides {
    bool hasExpandX = false; float expandX = 0.0f;
    bool hasExpandY = false; float expandY = 0.0f;
    bool hasColor = false; SDL_Color color = {0,0,0,0};
    bool hasRadius = false; int radius = 0;
    bool hasPadding = false; Vector2 padding = {0,0};
    bool hasDebugBounds = false; bool showBounds = false;

    // A quick way to clear live edits on a widget
    void Reset() { *this = WidgetOverrides(); }
};

inline std::unordered_map<std::string, WidgetOverrides> g_InspectorOverrides;

// --- 2. The Dynamic Inspector ---
void DrawWidgetInspector(Widget& widget, std::string parentPath = "root", int index = 0) {
    // Generate a unique ID based on the widget's ID, or its structural path (e.g., root[0][1])
    std::string uid = widget.id.empty() ? (parentPath + "[" + std::to_string(index) + "]") : widget.id;
    
    // --- APPLY OVERRIDES BEFORE DRAWING ---
    WidgetOverrides& ov = g_InspectorOverrides[uid];
    if (ov.hasExpandX) widget.expandX = ov.expandX;
    if (ov.hasExpandY) widget.expandY = ov.expandY;
    if (ov.hasColor) widget.style.color = ov.color;
    if (ov.hasRadius) widget.style.radius = ov.radius;
    if (ov.hasPadding) widget.style.padding = ov.padding;
    if (ov.hasDebugBounds) widget.debug.showBounds = ov.showBounds;

    // --- DRAW IMGUI HIERARCHY ---
    ImGui::PushID(uid.c_str());
    
    std::string displayName = widget.id.empty() ? ("Widget " + std::to_string(index)) : widget.id;
    
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
    if (widget.children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;

    if (ImGui::TreeNodeEx(displayName.c_str(), flags)) {
        
        // Show path / ID
        ImGui::TextDisabled("UID: %s", uid.c_str());
        ImGui::SameLine();
        if (ImGui::Button("Reset Overrides")) ov.Reset();

        // 1. Layout Adjustments
        ImGui::Text("Layout Options");
        if (ImGui::DragFloat("Expand X", &widget.expandX, 0.05f, 0.0f, 1.0f)) {
            ov.hasExpandX = true; ov.expandX = widget.expandX;
        }
        if (ImGui::DragFloat("Expand Y", &widget.expandY, 0.05f, 0.0f, 1.0f)) {
            ov.hasExpandY = true; ov.expandY = widget.expandY;
        }

        // 2. Style Adjustments
        if (ImGui::TreeNode("Style Properties")) {
            float col[4] = { widget.style.color.r / 255.0f, widget.style.color.g / 255.0f, 
                             widget.style.color.b / 255.0f, widget.style.color.a / 255.0f };
            if (ImGui::ColorEdit4("Color", col)) {
                ov.hasColor = true;
                ov.color = { (Uint8)(col[0]*255), (Uint8)(col[1]*255), (Uint8)(col[2]*255), (Uint8)(col[3]*255) };
                widget.style.color = ov.color; // Apply instantly to current frame
            }

            if (ImGui::SliderInt("Radius", &widget.style.radius, 0, 100)) {
                ov.hasRadius = true; ov.radius = widget.style.radius;
            }
            
            if (ImGui::DragInt2("Padding", &widget.style.padding.x, 1, 0, 200)) {
                ov.hasPadding = true; ov.padding = widget.style.padding;
            }
            ImGui::TreePop();
        }

        // 3. Specific Debugging
        if (ImGui::Checkbox("Highlight Bounds", &widget.debug.showBounds)) {
            ov.hasDebugBounds = true; ov.showBounds = widget.debug.showBounds;
        }

        // 4. Recursive Children
        if (!widget.children.empty()) {
            ImGui::Separator();
            for (size_t i = 0; i < widget.children.size(); i++) {
                DrawWidgetInspector(widget.children[i], uid, i);
            }
        }

        ImGui::TreePop();
    }
    ImGui::PopID();
}

int main(int argc, char *args[])
{
    // ====================== INITIALIZATION ======================
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    if (TTF_Init() == -1)
    {
        std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    IMG_Init(IMG_INIT_PNG);

    SDL_Window *window = SDL_CreateWindow(
        "Navigation Scaffold", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (!window)
    {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!renderer)
    {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    // ====================== ImGui INITIALIZATION ======================
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // ===================== FONT LOADING ===================
    auto loadFont = [](const std::string &path, int size) -> TTF_Font *
    {
        TTF_Font *font = TTF_OpenFont(path.c_str(), size);
        if (!font)
            std::cout << "Warning: Failed to load font: " << TTF_GetError()
                      << std::endl;

        std::cout << "Loaded font" << path << std::endl;
        return font;
    };
    TTF_Font *fontSpaceGrotesk_w700_24p =
        loadFont("assets/fonts/spacegrotesk/SpaceGrotesk-700.ttf", 24);
    TTF_Font *fontArial18p = loadFont("assets/fonts/Arial.ttf", 18);
    TTF_Font *fontArial20p = loadFont("assets/fonts/Arial.ttf", 20);
    TTF_Font *fontAwesome = loadFont("fontawesome/fa-solid-900.otf", 24);

    std::cout << "SDL + ImGui App started successfully!" << std::endl;

    // ====================== UI Kit Init ======================
    initUIKit();
    Cursors_Init();
    g_NextFocusedWidgetId = "navbar_home";

    // ====================== MAIN LOOP ======================
    bool quit = false;
    SDL_Event e;
    Uint32 lastTime = SDL_GetTicks();

    while (!quit)
    {
        // --- Input ---
        InputState input = gatherInputState(e, quit);

        // --- Timing ---
        Uint32 currentTime = SDL_GetTicks();
        float dt = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // ====================== ImGui NEW FRAME ======================
        ImGui_ImplSDL2_NewFrame();
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui::NewFrame();

        // ====================== YOUR CUSTOM UI ======================
        int ww, wh;
        SDL_GetWindowSize(window, &ww, &wh);

        using namespace Widgets;

        auto NavItem = [&](uint32_t iconCode, const std::string &label,
                           const std::string &id,
                           const WidgetNav &nav = {"", "", "", "", "", ""})
        {
            WidgetStyle style;
            style.color = {0, 0, 0, 0};
            style.padding = {0, 0};
            style.radius = 12;
            Widget w = RoundedBox({}, style,
                                  {Padding({16, 8}, Text(label, fontArial18p, {161, 161, 170, 255}))});
            w.id = id;

            return w
                .OnHover(id, 0.25f,
                         [](Widget &w, float t)
                         {
                             SetCursor(CursorType::Hand);
                             w.animateColor({255, 255, 255, 25}, t);
                             w.children[0].children[0].animateColor({255, 255, 255, 255}, t);
                         })
                .OnFocus(id, 0.25f,
                         [](Widget &w, float t)
                         {
                             w.animateColor({255, 255, 255, 50}, t);
                             w.children[0].children[0].animateColor({255, 255, 255, 255}, t);
                         })
                .WithNav(nav.up, nav.down, nav.left, nav.right, nav.next, nav.prev);
        };

        auto NavItemRounded = [&](uint32_t iconCode, const std::string &label,
                                  const std::string &id,
                                  const WidgetNav &nav = {"", "", "", "", "", ""})
        {
            WidgetStyle style;
            style.color = {0, 0, 0, 0};
            style.padding = {0, 0};
            style.radius = 22;

            Widget w = RoundedBox(
                {44, 44}, style,
                {Icon({iconCode}, fontAwesome, 18, {255, 255, 255, 255}, {}, 0)});
            w.id = id;

            return w
                .OnHover(id, 0.25f,
                         [](Widget &w, float t)
                         {
                             SetCursor(CursorType::Hand);
                             w.animateColor({255, 255, 255, 25}, t);
                             w.animateBorder({255,255,255,255},2,t);
                             w.children[0].animateColor({255, 255, 255, 255}, t);
                         })
                .OnFocus(id, 0.25f,
                         [](Widget &w, float t)
                         {
                             w.animateColor({255, 255, 255, 50}, t);
                             w.animateBorder({255,255,255,255},2,t);
                             w.children[0].animateColor({255, 255, 255, 255}, t);
                         })
                .WithNav(nav.up, nav.down, nav.left, nav.right, nav.next, nav.prev);
        };

        ResetCursor();

        WidgetStyle logoStyle;
        logoStyle.color = {0, 0, 0, 0};

        Widget screen = Scaffold(
            Column(
                MainAxisAlignment::Start, CrossAxisAlignment::Start, 0,
                {Text("Empty Scaffold - Ready", fontArial18p, {255, 255, 255, 255}),
                 Expanded(
                     0, 1,
                     Padding(
                         {60, 40},
                         Expanded(
                             0, 1,
                             Row(MainAxisAlignment::SpaceBetween,
                                 CrossAxisAlignment::Center, 0,
                                 {Row(MainAxisAlignment::Start,
                                      CrossAxisAlignment::Center, 30,
                                      {
                                          RoundedBox(
                                              {}, logoStyle,
                                              {
                                                  Text("Terebi",
                                                       fontSpaceGrotesk_w700_24p,
                                                       {255, 255, 255, 255}, {}),
                                              }),
                                          NavItem(FontAwesome::Home().codepoint,
                                                  "Home", "navbar_home",
                                                  {"", "", "navbar_settings",
                                                   "navbar_movies", "navbar_movies",
                                                   "navbar_settings"}),
                                          NavItem(FontAwesome::Home().codepoint,
                                                  "Movies", "navbar_movies",
                                                  {"", "", "navbar_home",
                                                   "navbar_shows", "navbar_shows",
                                                   "navbar_home"}),
                                          NavItem(FontAwesome::Home().codepoint,
                                                  "Shows", "navbar_shows",
                                                  {"", "", "navbar_movies",
                                                   "navbar_apps", "navbar_apps",
                                                   "navbar_movies"}),
                                          NavItem(FontAwesome::Home().codepoint,
                                                  "Apps", "navbar_apps",
                                                  {"", "", "navbar_shows",
                                                   "navbar_library",
                                                   "navbar_library",
                                                   "navbar_shows"}),
                                          NavItem(FontAwesome::Home().codepoint,
                                                  "Library", "navbar_library",
                                                  {"", "", "navbar_apps",
                                                   "navbar_search", "navbar_search",
                                                   "navbar_apps"}),
                                      }),
                                  Row(MainAxisAlignment::Start,
                                      CrossAxisAlignment::Center, 30,
                                      {NavItemRounded(
                                           FontAwesome::Search().codepoint,
                                           "Search", "navbar_search",
                                           {"", "", "navbar_library",
                                            "navbar_settings", "navbar_settings",
                                            "navbar_library"}),
                                       NavItemRounded(
                                           FontAwesome::Search().codepoint,
                                           "Search", "navbar_settings",
                                           {"", "", "navbar_search", "navbar_home",
                                            "navbar_search",
                                            "navbar_home"})})}))))}),
            ForegroundBlur(0, Stack({Image("assets/images/frieren.jpg", {ww, wh}), Box({ww, wh}, {{0, 0, 0, 205}})})));

        StartUIFrame();
        UpdateUIAnimations(dt);

        SDL_SetRenderDrawColor(renderer, 30, 30, 35, 255);
        SDL_RenderClear(renderer);

        screen.render(renderer, {0, 0, ww, wh}, input);

        // ====================== ImGui DEBUG WINDOW ======================
        ImGui::Begin("Debug Info");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Mouse: %d, %d", input.mouseX, input.mouseY);
        ImGui::Text("Focused: %s", g_FocusedWidgetId.c_str());
        ImGui::Text("Next Focus: %s", g_NextFocusedWidgetId.c_str());
        ImGui::Separator();

        ImGui::Checkbox("Enable Debug", &g_GlobalDebug.enabled);
        ImGui::Checkbox("Show Bounds", &g_GlobalDebug.showBounds);
        ImGui::Checkbox("Show Padding", &g_GlobalDebug.showPadding);
        ImGui::Checkbox("Show Spacing", &g_GlobalDebug.showSpacing);
        ImGui::Checkbox("Show Nav Arrows", &g_GlobalDebug.showNavArrows);

        ImGui::Separator();
        ImGui::Text("Colors:");

        // Color controls
        if (g_GlobalDebug.showBounds)
        {
            float boundsCol[4] = {g_GlobalDebug.boundsColor.r / 255.0f,
                                  g_GlobalDebug.boundsColor.g / 255.0f,
                                  g_GlobalDebug.boundsColor.b / 255.0f,
                                  g_GlobalDebug.boundsColor.a / 255.0f};
            if (ImGui::ColorEdit4("Bounds Color", boundsCol,
                                  ImGuiColorEditFlags_AlphaBar))
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
                                   g_GlobalDebug.paddingColor.a / 255.0f};
            if (ImGui::ColorEdit4("Padding Color", paddingCol,
                                  ImGuiColorEditFlags_AlphaBar))
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
                                   g_GlobalDebug.spacingColor.a / 255.0f};
            if (ImGui::ColorEdit4("Spacing Color", spacingCol,
                                  ImGuiColorEditFlags_AlphaBar))
            {
                g_GlobalDebug.spacingColor.r = (Uint8)(spacingCol[0] * 255.0f);
                g_GlobalDebug.spacingColor.g = (Uint8)(spacingCol[1] * 255.0f);
                g_GlobalDebug.spacingColor.b = (Uint8)(spacingCol[2] * 255.0f);
                g_GlobalDebug.spacingColor.a = (Uint8)(spacingCol[3] * 255.0f);
            }
        }
        ImGui::End();

        // ImGui::Begin("Widget Hierarchy Inspector");
        // DrawWidgetInspector(screen, "root", 0);
        // ImGui::End();
        // ====================== RENDER ImGui ======================
        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

        // ====================== PRESENT ======================
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

    std::cout << "Application closed cleanly." << std::endl;
    return 0;
}

// =============================================
// Helper Functions
// =============================================
void initUIKit()
{
    std::cout << "[UI Kit] Initialized successfully." << std::endl;
}

InputState gatherInputState(SDL_Event &e, bool &quit)
{
    InputState input = {};
    input.mouseClicked = false;
    input.mouseWheelX = 0.0f;
    input.mouseWheelY = 0.0f;
    input.keyPressed = SDLK_UNKNOWN;
    input.keyMod = 0;

    while (SDL_PollEvent(&e))
    {
        ImGui_ImplSDL2_ProcessEvent(&e);

        if (e.type == SDL_QUIT)
            quit = true;
        else if (e.type == SDL_MOUSEBUTTONDOWN &&
                 e.button.button == SDL_BUTTON_LEFT)
            input.mouseClicked = true;
        else if (e.type == SDL_MOUSEWHEEL)
        {
            input.mouseWheelX = (float)e.wheel.x;
            input.mouseWheelY = (float)e.wheel.y;
        }
        else if (e.type == SDL_KEYDOWN)
        {
            input.keyPressed = e.key.keysym.sym;
            input.keyMod = e.key.keysym.mod;
        }
    }

    SDL_GetMouseState(&input.mouseX, &input.mouseY);
    return input;
}