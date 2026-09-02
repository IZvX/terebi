#pragma once

#include "sdl_compat.h"
#include <SDL3_image/SDL_image.h>

#include <algorithm>
#include <functional>
#include <cmath>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_sdl3.h"
#include "../imgui/backends/imgui_impl_sdlrenderer3.h"
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

        Uint32 sdlWindowFlags = SDL_WINDOW_RESIZABLE;
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
        inline std::unordered_map<std::string, TTF_Font*> g_fontsByKey;
        inline std::unordered_map<std::string, std::string> g_fontIds;

        inline std::string FontKey(const std::string& path, int size)
        {
            return path + "#" + std::to_string(size);
        }

        // Text field focus event state
        inline std::vector<
            std::function<void(const std::string&, bool)>
        > g_onTextFieldFocusCallbacks;

        inline std::unordered_map<std::string, bool>
            g_previousTextFieldFocus;

        // Layers for overlay widgets like toasts
        struct Layer {
            std::string id;
            Widget widget;
        };

        inline std::vector<Layer> g_layers;
    }

    // Public API
    inline void OnTextFieldFocus(
        std::function<void(const std::string&, bool)> callback)
    {
        Detail::g_onTextFieldFocusCallbacks.push_back(std::move(callback));
    }

    inline bool IsTextFieldFocused()
    {
        return g_inputFocused;
    }

    inline void FireTextFieldFocusEvent(
        const std::string& id,
        bool focused)
    {
        auto it = Detail::g_previousTextFieldFocus.find(id);

        if (it == Detail::g_previousTextFieldFocus.end())
        {
            Detail::g_previousTextFieldFocus[id] = focused;
            return;
        }

        if (it->second == focused)
            return;

        it->second = focused;
        InvalidateUI();

        for (auto& callback : Detail::g_onTextFieldFocusCallbacks)
        {
            callback(id, focused);
        }
    }

    inline void AddLayer(Widget layer)
    {
        InvalidateUI();
        Detail::g_layers.push_back({"", std::move(layer)});
    }

    inline void AddLayer(Widget layer, const std::string &id)
    {
        InvalidateUI();
        if (id.empty())
        {
            AddLayer(std::move(layer));
            return;
        }

        for (auto &entry : Detail::g_layers)
        {
            if (entry.id == id)
            {
                entry.widget = std::move(layer);
                return;
            }
        }

        Detail::g_layers.push_back({id, std::move(layer)});
    }

    inline void RemoveLayer(const std::string &id)
    {
        InvalidateUI();
        auto it = std::remove_if(
            Detail::g_layers.begin(),
            Detail::g_layers.end(),
            [&](const Detail::Layer &entry) {
                return entry.id == id;
            });
        Detail::g_layers.erase(it, Detail::g_layers.end());
    }

    inline void ClearLayers()
    {
        InvalidateUI();
        Detail::g_layers.clear();
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

    inline const std::string &SecondaryFocusedWidget()
    {
        return g_SecondaryFocusedWidgetId;
    }

    inline void SetSecondaryFocus(const std::string &id)
    {
        g_NextSecondaryFocusedWidgetId = id;
        InvalidateUI();
    }

    inline void ClearSecondaryFocus()
    {
        g_NextSecondaryFocusedWidgetId.clear();
        InvalidateUI();
    }

    using ::NavAction;

    inline void ClearNavigationMappings()
    {
        ::ClearNavigationMappings();
    }

    inline void ResetDefaultNavigationMappings()
    {
        ::ResetDefaultNavigationMappings();
    }

    inline void MapNavigationKey(NavAction action, SDL_Keycode key, Uint16 requiredMod = 0, Uint16 rejectedMod = 0, const std::string &label = "")
    {
        ::MapNavigationKey(action, key, requiredMod, rejectedMod, label);
    }

    inline void MapNavigationMouseButton(NavAction action, Uint8 mouseButton, const std::string &label = "")
    {
        ::MapNavigationMouseButton(action, mouseButton, label);
    }

    inline void MapNavigationFlag(NavAction action, bool *flag, bool consume = true, const std::string &label = "")
    {
        ::MapNavigationFlag(action, flag, consume, label);
    }

    inline void MapNavigationPredicate(NavAction action, std::function<bool(const InputState &)> predicate, const std::string &label = "")
    {
        ::MapNavigationPredicate(action, std::move(predicate), label);
    }

    inline void MapNavigationCallback(NavAction action, std::function<bool()> callback, const std::string &label = "")
    {
        ::MapNavigationCallback(action, std::move(callback), label);
    }

    inline void OnNavigationAction(NavAction action, std::function<void()> callback)
    {
        ::OnNavigationAction(action, std::move(callback));
    }

    inline void OnBack(std::function<void()> callback)
    {
        ::OnNavigationAction(NavAction::Back, std::move(callback));
    }

    inline void OnForward(std::function<void()> callback)
    {
        ::OnNavigationAction(NavAction::Forward, std::move(callback));
    }

    inline void TriggerNavigationAction(NavAction action, const std::string &label = "")
    {
        ::TriggerNavigationAction(action, label);
    }

    inline void ClearNavigationActionCallbacks()
    {
        ::ClearNavigationActionCallbacks();
    }

    inline void ToggleWidgetInspector()
    {
        g_WidgetInspectorOpen = !g_WidgetInspectorOpen;
        InvalidateUI();
    }

    inline bool IsWidgetInspectorOpen()
    {
        return g_WidgetInspectorOpen;
    }

    inline const WidgetInspectorRecord *FindInspectorRecord(const std::string &id)
    {
        if (id.empty())
            return nullptr;

        auto it = std::find_if(
            g_WidgetInspectorRecords.begin(),
            g_WidgetInspectorRecords.end(),
            [&](const WidgetInspectorRecord &record) {
                return record.id == id;
            });

        return it == g_WidgetInspectorRecords.end() ? nullptr : &(*it);
    }

    inline void RenderInspectorTreeNode(
        const WidgetInspectorRecord &record,
        const std::vector<WidgetInspectorRecord> &records)
    {
        if (record.id.empty())
            return;

        bool hasChildren = false;
        for (const auto &candidate : records)
        {
            if (candidate.parentId == record.id)
            {
                hasChildren = true;
                break;
            }
        }

        ImGui::PushID(record.id.c_str());
        std::string label = std::string(WidgetTypeIcon(record.type)) + " " + record.id + " [" + record.type + "]";
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (!hasChildren)
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if (record.id == g_WidgetInspectorSelectedId)
            flags |= ImGuiTreeNodeFlags_Selected;

        bool open = ImGui::TreeNodeEx(label.c_str(), flags);
        if (ImGui::IsItemHovered())
            g_WidgetInspectorHoveredId = record.id;
        if (ImGui::IsItemClicked())
            g_WidgetInspectorSelectedId = record.id;

        if (open && hasChildren)
        {
            for (const auto &child : records)
            {
                if (child.parentId == record.id)
                    RenderInspectorTreeNode(child, records);
            }
            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    inline void RenderInspectorRecordProperties(const WidgetInspectorRecord &record)
    {
        ImGui::Text("ID: %s", record.id.c_str());
        ImGui::Text("Type: %s", record.type.c_str());
        ImGui::Text("Parent: %s", record.parentId.empty() ? "-" : record.parentId.c_str());
        ImGui::Text("Rect: %d, %d  %dx%d", record.rect.x, record.rect.y, record.rect.w, record.rect.h);
        ImGui::Text("Size: %.0f x %.0f", (float)record.size.x, (float)record.size.y);
        ImGui::Text("Children: %d", record.childCount);
        ImGui::Text("Focused: %s", record.focused ? "true" : "false");
        ImGui::Text("Hovered: %s", record.hovered ? "true" : "false");
        ImGui::Text("Disabled: %s", record.disabled ? "true" : "false");
    }

    inline void RenderInspectorStyleEditor(const WidgetInspectorRecord &record)
    {
        WidgetInspectorOverride &ov = g_WidgetInspectorOverrides[record.id];

        if (ImGui::Checkbox("Override disabled", &ov.disabledSet))
            InvalidateUI();
        if (ov.disabledSet)
        {
            if (ImGui::Checkbox("Disabled", &ov.disabled))
                InvalidateUI();
        }

        ImGui::Separator();
        if (ImGui::Checkbox("Override color", &ov.colorSet))
            InvalidateUI();
        if (ov.colorSet)
        {
            if (ov.color.a == 0 && ov.color.r == 0 && ov.color.g == 0 && ov.color.b == 0)
                ov.color = record.color;
            float color[4] = {
                ov.color.r / 255.0f,
                ov.color.g / 255.0f,
                ov.color.b / 255.0f,
                ov.color.a / 255.0f
            };
            if (ImGui::ColorEdit4("Color", color))
            {
                ov.color = {
                    (Uint8)(std::clamp(color[0], 0.0f, 1.0f) * 255),
                    (Uint8)(std::clamp(color[1], 0.0f, 1.0f) * 255),
                    (Uint8)(std::clamp(color[2], 0.0f, 1.0f) * 255),
                    (Uint8)(std::clamp(color[3], 0.0f, 1.0f) * 255)
                };
                InvalidateUI();
            }
        }

        ImGui::Separator();
        if (ImGui::Checkbox("Override position", &ov.positionSet))
            InvalidateUI();
        if (ov.positionSet)
        {
            float pos[2] = {(float)ov.position.x, (float)ov.position.y};
            if (ImGui::InputFloat2("Position", pos))
            {
                ov.position = {(int)pos[0], (int)pos[1]};
                InvalidateUI();
            }
        }

        if (ImGui::Checkbox("Override radius", &ov.radiusSet))
            InvalidateUI();
        if (ov.radiusSet)
        {
            if (ImGui::SliderInt("Radius", &ov.radius, 0, 64))
                InvalidateUI();
        }

        ImGui::Separator();
        if (ImGui::Button("Clear selected overrides"))
        {
            g_WidgetInspectorOverrides.erase(record.id);
            InvalidateUI();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear all"))
        {
            g_WidgetInspectorOverrides.clear();
            InvalidateUI();
        }
    }

    inline void RenderWidgetInspectorPanel(int height)
    {
        if (!g_WidgetInspectorOpen)
            return;

        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2((float)g_WidgetInspectorWidth, (float)height), ImGuiCond_Always);
        ImGui::Begin("Nimble Inspector", &g_WidgetInspectorOpen,
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        ImGui::Text("Focused: %s", g_FocusedWidgetId.c_str());
        ImGui::Text("Secondary: %s", g_SecondaryFocusedWidgetId.empty() ? "-" : g_SecondaryFocusedWidgetId.c_str());
        ImGui::Separator();

        g_WidgetInspectorHoveredId.clear();
        const float drawerHeight = std::max(220.0f, height * 0.38f);
        if (ImGui::BeginChild("widget_tree", ImVec2(0, -drawerHeight), true))
        {
            const WidgetInspectorRecord *root = FindInspectorRecord("root");
            if (root && ImGui::TreeNodeEx("Root", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth))
            {
                RenderInspectorTreeNode(*root, g_WidgetInspectorRecords);
                for (const auto &record : g_WidgetInspectorRecords)
                {
                    if (!record.id.empty() && record.id != "root" && record.parentId.empty())
                    {
                        bool isLayerRoot = false;
                        for (const auto &layer : Detail::g_layers)
                        {
                            if ((!layer.id.empty() && layer.id == record.id) ||
                                (!layer.widget.id.empty() && layer.widget.id == record.id))
                            {
                                isLayerRoot = true;
                                break;
                            }
                        }
                        if (!isLayerRoot)
                            RenderInspectorTreeNode(record, g_WidgetInspectorRecords);
                    }
                }
                ImGui::TreePop();
            }

            if (ImGui::TreeNodeEx("Layers", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth))
            {
                if (Detail::g_layers.empty())
                    ImGui::TextDisabled("No layers");
                for (const auto &layer : Detail::g_layers)
                {
                    std::string layerRootId = !layer.id.empty() ? layer.id : layer.widget.id;
                    const WidgetInspectorRecord *layerRecord = FindInspectorRecord(layerRootId);
                    if (!layerRecord && !layer.widget.id.empty())
                        layerRecord = FindInspectorRecord(layer.widget.id);
                    if (layerRecord)
                        RenderInspectorTreeNode(*layerRecord, g_WidgetInspectorRecords);
                    else
                        ImGui::TextDisabled("%s", layerRootId.empty() ? "(anonymous layer)" : layerRootId.c_str());
                }
                ImGui::TreePop();
            }
        }
        ImGui::EndChild();

        ImGui::Separator();
        const WidgetInspectorRecord *selected = FindInspectorRecord(g_WidgetInspectorSelectedId);

        if (!selected)
        {
            ImGui::TextDisabled("Select a widget to inspect it.");
            ImGui::End();
            return;
        }

        if (ImGui::BeginTabBar("inspector_drawer_tabs"))
        {
            if (ImGui::BeginTabItem("Properties"))
            {
                RenderInspectorRecordProperties(*selected);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Style"))
            {
                RenderInspectorStyleEditor(*selected);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    inline void RenderWidgetInspectorHighlights(SDL_Renderer *renderer)
    {
        if (!g_WidgetInspectorOpen)
            return;

        auto drawRecord = [&](const std::string &id, SDL_Color color, int inset) {
            const WidgetInspectorRecord *record = FindInspectorRecord(id);
            if (!record || record->rect.w <= 0 || record->rect.h <= 0)
                return;

            SDL_Rect rect = record->rect;
            rect.x -= inset;
            rect.y -= inset;
            rect.w += inset * 2;
            rect.h += inset * 2;

            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            SDL_RenderDrawRect(renderer, &rect);
            rect.x += 1;
            rect.y += 1;
            rect.w -= 2;
            rect.h -= 2;
            if (rect.w > 0 && rect.h > 0)
                SDL_RenderDrawRect(renderer, &rect);
        };

        drawRecord(g_WidgetInspectorHoveredId, {80, 220, 255, 220}, 2);
        drawRecord(g_WidgetInspectorSelectedId, {255, 210, 90, 240}, 4);
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

            SDL_SetHint(SDL_HINT_APP_ID, "terebi");
            SDL_SetHint(SDL_HINT_APP_NAME, "Terebi");

            if (!SDL_Init(SDL_INIT_VIDEO))
                return false;
            if (!TTF_Init())
                return false;

            window_ = SDL_CreateWindow(
                config_.title.c_str(),
                config_.width,
                config_.height,
                config_.sdlWindowFlags);
            if (!window_)
                return false;

            SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

            SDL_SetHint("SDL_VIDEO_EGL_ALLOW_TRANSPARENCY", "1");
            SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

            renderer_ = SDL_CreateRenderer(window_, nullptr);

            std::cout << "[NIMBLE] Active Renderer Driver: " 
                << (renderer_ ? SDL_GetRendererName(renderer_) : "NULL") 
                << std::endl;
            if (!renderer_)
                return false;
                
            if (config_.enableVSync)
                SDL_SetRenderVSync(renderer_, 1);

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGui_ImplSDL3_InitForSDLRenderer(window_, renderer_);
            ImGui_ImplSDLRenderer3_Init(renderer_);

            SDL_StartTextInput(window_);

            Cursors_Init();
            g_NextFocusedWidgetId = "navbar_home";
            if (g_NavigationBindings.empty())
                ResetDefaultNavigationMappings();

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
            Uint64 lastPerfCounter = SDL_GetPerformanceCounter();
            const double perfFreq = static_cast<double>(SDL_GetPerformanceFrequency());

            // Rolling average profiling counters
            int frameCounter = 0;
            double accInput = 0.0, accBuild = 0.0, accRender = 0.0, accImGui = 0.0, accPresent = 0.0, accTotal = 0.0;
            Uint64 fpsTimer = SDL_GetTicks();

            while (!quit)
            {
                Uint64 t0 = SDL_GetPerformanceCounter();

                // 1. Calculate Delta Time (high precision)
                float dt = static_cast<float>((t0 - lastPerfCounter) / perfFreq);
                lastPerfCounter = t0;

                // 2. Gather Input & Invalidation Check
                const bool hasAnimations = HasActiveUIAnimations();
                InputState input = GatherInput(quit, hasAnimations);
                Uint64 t1 = SDL_GetPerformanceCounter();

                // 3. Clear per-frame transient buffers (Fixes memory leaks)
                g_WidgetInspectorRecords.clear();
                Detail::g_layers.clear();

                // 4. Update UI Framework Animations & Cursors
                StartUIFrame();
                UpdateUIAnimations(dt);
                ResetCursor();

                // 5. ImGui Frame Setup
                ImGui_ImplSDL3_NewFrame();
                ImGui_ImplSDLRenderer3_NewFrame();
                ImGui::NewFrame();

                int width = 0;
                int height = 0;
                SDL_GetWindowSize(window_, &width, &height);
                const int inspectorOffset = g_WidgetInspectorOpen ? g_WidgetInspectorWidth : 0;
                const int appWidth = std::max(1, width - inspectorOffset);

                // 6. Clear Frame Buffer (transparent background)
                SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 0);
                SDL_RenderClear(renderer_);

                // 7. App Tree Construction
                Uint64 t2 = t1;
                Uint64 t3 = t1;
                if (app_)
                {
                    Widget root = app_->Build(appWidth, height, dt);
                    t2 = SDL_GetPerformanceCounter();

                    root.id = root.id.empty() ? "root" : root.id;
                    root.render(renderer_, {inspectorOffset, 0, appWidth, height}, input);
                    t3 = SDL_GetPerformanceCounter();
                }

                // 8. Render Overlay Layers (Toasts, Modals)
                for (auto& layer : Detail::g_layers)
                {
                    layer.widget.render(renderer_, {inspectorOffset, 0, appWidth, height}, input);
                }

                RenderDebugNavigationOverlay(renderer_);

#ifndef NDEBUG
                for (auto &debugCb : Detail::g_debugCallbacks)
                    debugCb();
#endif

                // 9. Inspector & ImGui Draw Calls
                RenderWidgetInspectorPanel(height);
                RenderWidgetInspectorHighlights(renderer_);

                ImGui::Render();
                ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer_);
                Uint64 t4 = SDL_GetPerformanceCounter();

                // 10. GPU Presentation / Buffer Swap
                SDL_RenderPresent(renderer_);
                Uint64 t5 = SDL_GetPerformanceCounter();

                EndUIFrame(renderer_);

                // 11. Profile Accumulation & Display
                double msInput   = ((t1 - t0) * 1000.0) / perfFreq;
                double msBuild   = ((t2 - t1) * 1000.0) / perfFreq;
                double msRender  = ((t3 - t2) * 1000.0) / perfFreq;
                double msImGui   = ((t4 - t3) * 1000.0) / perfFreq;
                double msPresent = ((t5 - t4) * 1000.0) / perfFreq;
                double msTotal   = ((t5 - t0) * 1000.0) / perfFreq;

                accInput += msInput;
                accBuild += msBuild;
                accRender += msRender;
                accImGui += msImGui;
                accPresent += msPresent;
                accTotal += msTotal;
                frameCounter++;

                Uint32 currentTicks = SDL_GetTicks();
                if (currentTicks - fpsTimer >= 1000)
                {
                    double avgFPS     = frameCounter * 1000.0 / (currentTicks - fpsTimer);
                    double avgInput   = accInput / frameCounter;
                    double avgBuild   = accBuild / frameCounter;
                    double avgRender  = accRender / frameCounter;
                    double avgImGui   = accImGui / frameCounter;
                    double avgPresent = accPresent / frameCounter;
                    double avgTotal   = accTotal / frameCounter;

                    std::cout << "[FPS: " << static_cast<int>(avgFPS) << "] "
                              << "Build: " << avgBuild << "ms | "
                              << "Render: " << avgRender << "ms | "
                              << "Present: " << avgPresent << "ms | "
                              << "Input: " << avgInput << "ms | "
                              << "ImGui: " << avgImGui << "ms | "
                              << "FrameTime: " << avgTotal << "ms\n";

                    frameCounter = 0;
                    accInput = accBuild = accRender = accImGui = accPresent = accTotal = 0.0;
                    fpsTimer = currentTicks;
                }
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

            SDL_StopTextInput(window_);
            ImGui_ImplSDLRenderer3_Shutdown();
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();

            Cursors_Quit();
            if (renderer_)
                SDL_DestroyRenderer(renderer_);
            if (window_)
                SDL_DestroyWindow(window_);
            TTF_Quit();
            SDL_Quit();

            renderer_ = nullptr;
            window_ = nullptr;
            initialized_ = false;
        }

    private:
        InputState GatherInput(bool &quit, bool hasActiveAnimations)
        {
            InputState input = {};
            input.keyPressed = SDLK_UNKNOWN;

            SDL_Event event;

            // When idle (no animations), wait up to 16ms for an event to avoid spinning 100% CPU
            if (!hasActiveAnimations && !g_UINeedsRedraw && !g_WidgetInspectorOpen)
            {
                if (SDL_WaitEventTimeout(&event, 16))
                {
                    ProcessSingleEvent(event, input, quit);
                }
            }

            // Drain any remaining queued events
            while (SDL_PollEvent(&event))
            {
                ProcessSingleEvent(event, input, quit);
            }

            float mouseX = 0.0f;
            float mouseY = 0.0f;
            SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mouseX, &mouseY);
            input.mouseX = static_cast<int>(mouseX);
            input.mouseY = static_cast<int>(mouseY);
            input.leftMouseDown = (buttons & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0;
            input.rightMouseDown = (buttons & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT)) != 0;

            EvaluateNavigationMappings(input);

            return input;
        }

        void ProcessSingleEvent(const SDL_Event &event, InputState &input, bool &quit)
        {
            input.anyEvent = true;
            InvalidateUI();
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT)
            {
                quit = true;
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                input.mouseButtonPressed = event.button.button;
                if (event.button.button == SDL_BUTTON_LEFT)
                    input.mouseClicked = true;
                if (event.button.button == SDL_BUTTON_RIGHT)
                    input.rightMouseClicked = true;
            }
            else if (event.type == SDL_EVENT_MOUSE_WHEEL)
            {
                input.mouseWheelX = event.wheel.x;
                input.mouseWheelY = event.wheel.y;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN)
            {
                input.keyPressed = event.key.key;
                input.keyMod = event.key.mod;
                if (event.key.key == SDLK_F12)
                    g_WidgetInspectorOpen = !g_WidgetInspectorOpen;
                input.backspacePressed = (event.key.key == SDLK_BACKSPACE);
                input.deletePressed = (event.key.key == SDLK_DELETE);
                input.leftPressed = (event.key.key == SDLK_LEFT);
                input.rightPressed = (event.key.key == SDLK_RIGHT);
                input.enterPressed = (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER);
            }
            else if (event.type == SDL_EVENT_TEXT_INPUT)
            {
                input.textInput += event.text.text;
            }
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