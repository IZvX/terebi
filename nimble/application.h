#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <functional>
#include <cmath>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_sdl2.h"
#include "../imgui/backends/imgui_impl_sdlrenderer2.h"
#include "nimble.cpp"
#include "utils/cursors.h"

namespace Nimble
{
    enum class RendererBackend
    {
        SDL
    };

    struct ApplicationConfig
    {
        std::string title = "Nimble Application";
        int width = 1280;
        int height = 720;
        RendererBackend renderer = RendererBackend::SDL;

        Uint32 sdlWindowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
        Uint32 sdlRendererFlags = SDL_RENDERER_ACCELERATED;
        bool enableVSync = true;
        bool enableGPU = false;
    };

    struct FontRequest
    {
        std::string id;
        std::string path;
        int size = 16;
    };

    namespace Detail
    {
        inline bool g_initialized = false;
        inline std::vector<std::function<void()>> g_onInitCallbacks;
        inline std::vector<std::function<void()>> g_onShutdownCallbacks;
        inline std::vector<std::function<void()>> g_debugCallbacks;
        inline std::unordered_map<std::string, TTF_Font *> g_fontsByKey;
        inline std::unordered_map<std::string, std::string> g_fontIds;

        inline std::string FontKey(const std::string &path, int size)
        {
            return path + "#" + std::to_string(size);
        }
    }

    inline void Init()
    {
        Detail::g_initialized = true;
    }

    inline void OnInit(std::function<void()> cb)
    {
        Detail::g_onInitCallbacks.push_back(std::move(cb));
    }

    inline void OnShutdown(std::function<void()> cb)
    {
        Detail::g_onShutdownCallbacks.push_back(std::move(cb));
    }

    inline TTF_Font *LoadFont(const std::string &path, int size)
    {
        const std::string key = Detail::FontKey(path, size);
        auto it = Detail::g_fontsByKey.find(key);
        if (it != Detail::g_fontsByKey.end())
            return it->second;

        TTF_Font *font = TTF_OpenFont(path.c_str(), size);
        if (!font)
            return nullptr;

        Detail::g_fontsByKey[key] = font;
        return font;
    }

    inline TTF_Font *LoadFont(const FontRequest &request)
    {
        TTF_Font *font = LoadFont(request.path, request.size);
        if (font && !request.id.empty())
            Detail::g_fontIds[request.id] = Detail::FontKey(request.path, request.size);
        return font;
    }

    inline TTF_Font *Font(const std::string &path, int size)
    {
        return LoadFont(path, size);
    }

    inline TTF_Font *FontById(const std::string &id)
    {
        auto it = Detail::g_fontIds.find(id);
        if (it == Detail::g_fontIds.end())
            return nullptr;
        auto fit = Detail::g_fontsByKey.find(it->second);
        return fit == Detail::g_fontsByKey.end() ? nullptr : fit->second;
    }

    inline const std::string &FocusedWidget()
    {
        return g_FocusedWidgetId;
    }

    class IApp
    {
    public:
        virtual ~IApp() = default;
        virtual void OnStart() {}
        virtual Widget Build(int width, int height, float deltaTime) = 0;
    };

    class Application
    {
    public:
        Application(ApplicationConfig config, std::unique_ptr<IApp> app)
            : config_(std::move(config)), app_(std::move(app)) {}

        ~Application()
        {
            Shutdown();
        }

        bool Initialize()
        {
            if (!Detail::g_initialized)
                Init();

            if (SDL_Init(SDL_INIT_VIDEO) < 0)
                return false;
            if (TTF_Init() == -1)
                return false;
            if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0)
                return false;

            Uint32 rendererFlags = config_.sdlRendererFlags;
            if (config_.enableVSync)
                rendererFlags |= SDL_RENDERER_PRESENTVSYNC;

            window_ = SDL_CreateWindow(
                config_.title.c_str(),
                SDL_WINDOWPOS_CENTERED,
                SDL_WINDOWPOS_CENTERED,
                config_.width,
                config_.height,
                config_.sdlWindowFlags);
            if (!window_)
                return false;

            renderer_ = SDL_CreateRenderer(window_, -1, rendererFlags);
            if (!renderer_)
                return false;

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGui_ImplSDL2_InitForSDLRenderer(window_, renderer_);
            ImGui_ImplSDLRenderer2_Init(renderer_);

            Cursors_Init();
            g_NextFocusedWidgetId = "navbar_home";

            for (auto &cb : Detail::g_onInitCallbacks)
                cb();
            if (app_)
                app_->OnStart();

            initialized_ = true;
            return true;
        }

        void Run()
        {
            if (!initialized_)
                return;

            bool quit = false;
            Uint32 lastTicks = SDL_GetTicks();
            while (!quit)
            {
                InputState input = GatherInput(quit);
                Uint32 now = SDL_GetTicks();
                float dt = static_cast<float>(now - lastTicks) / 1000.0f;
                lastTicks = now;

                StartUIFrame();
                UpdateUIAnimations(dt);
                ResetCursor();

                ImGui_ImplSDL2_NewFrame();
                ImGui_ImplSDLRenderer2_NewFrame();
                ImGui::NewFrame();

                int width = 0;
                int height = 0;
                SDL_GetWindowSize(window_, &width, &height);

                SDL_SetRenderDrawColor(renderer_, 30, 30, 35, 255);
                SDL_RenderClear(renderer_);

                if (app_)
                {
                    Widget root = app_->Build(width, height, dt);
                    root.render(renderer_, {0, 0, width, height}, input);
                }

                RenderDebugNavigationOverlay(renderer_);

#ifndef NDEBUG
                for (auto &debugCb : Detail::g_debugCallbacks)
                    debugCb();
#endif

                ImGui::Render();
                ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer_);
                SDL_RenderPresent(renderer_);
            }
        }

        void Shutdown()
        {
            if (!initialized_)
                return;

            for (auto &cb : Detail::g_onShutdownCallbacks)
                cb();

            for (auto &entry : Detail::g_fontsByKey)
            {
                if (entry.second)
                    TTF_CloseFont(entry.second);
            }
            Detail::g_fontsByKey.clear();
            Detail::g_fontIds.clear();

            ImGui_ImplSDLRenderer2_Shutdown();
            ImGui_ImplSDL2_Shutdown();
            ImGui::DestroyContext();

            Cursors_Quit();
            if (renderer_)
                SDL_DestroyRenderer(renderer_);
            if (window_)
                SDL_DestroyWindow(window_);
            IMG_Quit();
            TTF_Quit();
            SDL_Quit();

            renderer_ = nullptr;
            window_ = nullptr;
            initialized_ = false;
        }

    private:
        InputState GatherInput(bool &quit)
        {
            InputState input = {};
            input.keyPressed = SDLK_UNKNOWN;

            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                input.anyEvent = true;
                ImGui_ImplSDL2_ProcessEvent(&event);
                if (event.type == SDL_QUIT)
                    quit = true;
                else if (event.type == SDL_MOUSEBUTTONDOWN)
                {
                    if (event.button.button == SDL_BUTTON_LEFT)
                        input.mouseClicked = true;
                    if (event.button.button == SDL_BUTTON_RIGHT)
                        input.rightMouseClicked = true;
                }
                else if (event.type == SDL_MOUSEWHEEL)
                {
                    input.mouseWheelX = static_cast<float>(event.wheel.x);
                    input.mouseWheelY = static_cast<float>(event.wheel.y);
                }
                else if (event.type == SDL_KEYDOWN)
                {
                    input.keyPressed = event.key.keysym.sym;
                    input.keyMod = event.key.keysym.mod;
                    input.backspacePressed = (event.key.keysym.sym == SDLK_BACKSPACE);
                    input.deletePressed = (event.key.keysym.sym == SDLK_DELETE);
                    input.leftPressed = (event.key.keysym.sym == SDLK_LEFT);
                    input.rightPressed = (event.key.keysym.sym == SDLK_RIGHT);
                    input.enterPressed = (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER);
                }
                else if (event.type == SDL_TEXTINPUT)
                {
                    input.textInput += event.text.text;
                }
            }

            Uint32 buttons = SDL_GetMouseState(&input.mouseX, &input.mouseY);
            input.leftMouseDown = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
            input.rightMouseDown = (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;

            return input;
        }

        ApplicationConfig config_;
        std::unique_ptr<IApp> app_;
        SDL_Window *window_ = nullptr;
        SDL_Renderer *renderer_ = nullptr;
        bool initialized_ = false;
    };
} // namespace Nimble

namespace NimbleDebug
{
    inline void ImGui(std::function<void()> callback)
    {
        Nimble::Detail::g_debugCallbacks.push_back(std::move(callback));
    }
}
