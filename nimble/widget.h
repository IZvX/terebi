#pragma once
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>
#include <string_view>
#include <functional>
#include <climits>
#include <iostream>

// --- Global Developer Settings ---
struct GlobalSettings
{
    // 1. Layout & Metrics
    bool showWidgetIds = false;         // 1.1 Overlay Widget IDs
    bool showClipRects = false;         // 1.2 Clip Rect Visualization
    bool showFlexWeights = false;       // 1.3 Flex-Weight Labels

    // 2. Interaction & State
    bool showFocusLoop = false;         // 2.1 Focus Loop Visualizer
    bool highlightInputCapture = false; // 2.2 Input "Capture" Highlight

    // 3. Performance & Rendering
    bool slowAnimations = false;        // 3.1 Global Animation Timescale
    bool paintFlashMode = false;        // 3.2 Paint-Flash Mode
    bool showFPSOverlay = false;        // 3.4 FPS & DeltaTime Overlay

    // 4. Logic & Data
    bool triggerStateReset = false;     // 4.1 State Reset
};

inline GlobalSettings g_Settings;

// --- Invalidation & Dirty Drawing State ---
inline bool g_UINeedsRedraw = true;
inline std::vector<SDL_Rect> g_DirtyRects;

inline void InvalidateUI()
{
    g_UINeedsRedraw = true;
}

inline void MarkDirtyRect(const SDL_Rect &rect)
{
    g_UINeedsRedraw = true;
    if (rect.w > 0 && rect.h > 0)
        g_DirtyRects.push_back(rect);
}

inline void ClearDirtyRects()
{
    g_DirtyRects.clear();
}

inline bool GetCombinedDirtyRect(SDL_Rect &outRect)
{
    if (g_DirtyRects.empty())
        return false;

    SDL_Rect u = g_DirtyRects[0];
    for (size_t i = 1; i < g_DirtyRects.size(); ++i)
    {
        SDL_GetRectUnion(&u, &g_DirtyRects[i], &u);
    }
    outRect = u;
    return true;
}

inline std::string g_FocusedWidgetId = "";
inline std::string g_NextFocusedWidgetId = "";
inline std::string g_SecondaryFocusedWidgetId = "";
inline std::string g_NextSecondaryFocusedWidgetId = "";
inline bool g_inputFocused = false;
inline std::unordered_map<std::string, SDL_Rect> g_WidgetRects;
inline bool g_DebugNavigation = false;
inline std::unordered_map<std::string, bool> g_DisabledStates;

inline std::string ActiveTextFieldId()
{
    return !g_FocusedWidgetId.empty() ? g_FocusedWidgetId : g_NextFocusedWidgetId;
}

inline bool InsertTextIntoFocusedTextField(const std::string &value)
{
    InvalidateUI();
    return InsertTextIntoTextField(ActiveTextFieldId(), value);
}

inline bool BackspaceFocusedTextField()
{
    InvalidateUI();
    return BackspaceTextField(ActiveTextFieldId());
}

struct WidgetInspectorRecord
{
    std::string id;
    std::string parentId;
    std::string type;
    SDL_Rect rect = {0, 0, 0, 0};
    Vector2 size = {0, 0};
    SDL_Color color = {0, 0, 0, 0};
    int radius = 0;
    int borderWidth = 0;
    bool disabled = false;
    bool focused = false;
    bool hovered = false;
    int childCount = 0;
};

struct WidgetInspectorOverride
{
    bool disabledSet = false;
    bool disabled = false;
    bool colorSet = false;
    SDL_Color color = {0, 0, 0, 0};
    bool positionSet = false;
    Vector2 position = {0, 0};
    bool radiusSet = false;
    int radius = 0;
};

inline bool g_WidgetInspectorOpen = false;
inline std::string g_WidgetInspectorSelectedId = "";
inline std::string g_WidgetInspectorHoveredId = "";
inline int g_WidgetInspectorWidth = 360;
inline std::vector<WidgetInspectorRecord> g_WidgetInspectorRecords;
inline std::unordered_map<std::string, WidgetInspectorOverride> g_WidgetInspectorOverrides;

inline const char *GuessWidgetType(const std::string &id, bool hasText, int childCount)
{
    if (hasText) return "TextField";
    if (id.find("toggle") != std::string::npos || id.find("_switch") != std::string::npos) return "Toggle";
    if (id.find("btn") != std::string::npos || id.find("button") != std::string::npos) return "Button";
    if (id.find("drawer") != std::string::npos) return "Drawer";
    if (id.find("navbar") != std::string::npos) return "Nav";
    return childCount > 0 ? "Container" : "Widget";
}

inline const char *WidgetTypeIcon(const std::string &type)
{
    if (type == "TextField") return "T";
    if (type == "Toggle") return "S";
    if (type == "Button") return "B";
    if (type == "Drawer") return "D";
    if (type == "Nav") return "N";
    if (type == "Container") return "C";
    return "W";
}
    
// --- Navigation Direction Enum ---
enum class NavDir : uint8_t
{
    Up,
    Down,
    Left,
    Right,
    Next,
    Prev
};

// --- Navigation Struct ---
struct WidgetNav
{
    std::string up = "";
    std::string down = "";
    std::string left = "";
    std::string right = "";
    std::string next = "";
    std::string prev = "";

    const std::string& get(NavDir dir) const
    {
        switch (dir) {
            case NavDir::Up:    return up;
            case NavDir::Down:  return down;
            case NavDir::Left:  return left;
            case NavDir::Right: return right;
            case NavDir::Next:  return next;
            case NavDir::Prev:  return prev;
        }
        static const std::string empty = "";
        return empty;
    }
};

inline std::unordered_map<std::string, WidgetNav> g_NavigationLinks;

inline void NavLog(const std::string &from, const char *dir, const std::string &to)
{
    if (!g_DebugNavigation) return;
    std::cout << "[NAV] " << from << " -> " << dir << " -> " << (to.empty() ? "NONE" : to) << "\n";
}

inline std::string ResolveNavTarget(const std::string& startId, NavDir dir)
{
    auto it = g_NavigationLinks.find(startId);
    if (it == g_NavigationLinks.end()) return "";

    std::string currentTarget = it->second.get(dir);

    constexpr int MAX_HOPS = 16;
    int hops = 0;

    while (!currentTarget.empty() && hops++ < MAX_HOPS)
    {
        auto disIt = g_DisabledStates.find(currentTarget);
        if (disIt == g_DisabledStates.end() || !disIt->second)
        {
            return currentTarget;
        }

        auto nextIt = g_NavigationLinks.find(currentTarget);
        if (nextIt == g_NavigationLinks.end()) break;

        currentTarget = nextIt->second.get(dir);
    }

    return currentTarget;
}

inline void MoveFocusIfTargetExists(const std::string &fromId, NavDir dir, const char *label)
{
    const std::string target = ResolveNavTarget(fromId, dir);
    if (!target.empty())
    {
        g_NextFocusedWidgetId = target;
        InvalidateUI();
    }
    if (g_DebugNavigation)
        NavLog(fromId, label, target);
}

inline void MoveSecondaryFocusIfTargetExists(const std::string &fromId, NavDir dir, const char *label)
{
    const std::string target = ResolveNavTarget(fromId, dir);
    if (!target.empty())
    {
        g_NextSecondaryFocusedWidgetId = target;
        InvalidateUI();
    }
    if (g_DebugNavigation)
        NavLog(fromId, label, target);
}

// Check if any UI animation is actively running
inline bool HasActiveUIAnimations()
{
    for (const auto &pair : g_UIState)
    {
        const AnimState &anim = pair.second;
        if ((anim.hoverProgress > 0.0f && anim.hoverProgress < 1.0f) ||
            (anim.focusProgress > 0.0f && anim.focusProgress < 1.0f) ||
            anim.clickProgress > 0.0f ||
            (anim.manualProgress > 0.0f && anim.manualProgress < 1.0f) ||
            anim.isClicked || anim.isManuallyAnimated)
        {
            return true;
        }
    }
    return false;
}

// --- Debugging Structures ---
struct WidgetDebug
{
    bool enabled = false;
    bool showBounds = false;
    bool showPadding = false;
    bool showSpacing = false;
    bool showExpanded = false;
    bool showRow = false;
    bool showColumn = false;
    bool showNavArrows = false;
    bool focusOnlyNavArrows = true;
    bool childrenInherit = false;

    SDL_Color boundsColor = {255, 0, 0, 255};
    SDL_Color paddingColor = {170, 255, 170, 120};
    SDL_Color spacingColor = {255, 0, 255, 150};
};

inline WidgetDebug g_GlobalDebug = {false, false, false, false, false};

enum class DebugNavDrawKind
{
    DirectionalArrow,
    NextOutline,
    PrevOutline
};

struct DebugNavDrawRequest
{
    std::string widgetId;
    NavDir direction;
    SDL_Rect rect = {0, 0, 0, 0};
    SDL_Color color = {255, 255, 255, 255};
    DebugNavDrawKind kind = DebugNavDrawKind::DirectionalArrow;
};

inline std::vector<DebugNavDrawRequest> g_DebugNavDrawQueue;

namespace DebugDraw
{
    inline void DrawDiagonalRect(SDL_Renderer *r, SDL_Rect rect, SDL_Color color)
    {
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, color.r, color.g, color.b, 60);
        SDL_RenderFillRect(r, &rect);
        SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
        for (int i = -rect.h; i < rect.w; i += 8)
            SDL_RenderDrawLine(r, rect.x + i, rect.y, rect.x + i + rect.h, rect.y + rect.h);
        SDL_RenderDrawRect(r, &rect);
    }

    inline void DrawBoundsCross(SDL_Renderer *r, SDL_Rect rect, SDL_Color color)
    {
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
        SDL_RenderDrawRect(r, &rect);
        SDL_RenderDrawLine(r, rect.x, rect.y, rect.x + rect.w, rect.y + rect.h);
        SDL_RenderDrawLine(r, rect.x + rect.w, rect.y, rect.x, rect.y + rect.h);
    }

    inline void DrawDashedRect(SDL_Renderer *r, const SDL_Rect &rect, SDL_Color color, int dashLen = 6, int gapLen = 4)
    {
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);

        for (int x = rect.x; x < rect.x + rect.w; x += dashLen + gapLen)
        {
            SDL_RenderDrawLine(r, x, rect.y, std::min(x + dashLen, rect.x + rect.w), rect.y);
            SDL_RenderDrawLine(r, x, rect.y + rect.h, std::min(x + dashLen, rect.x + rect.w), rect.y + rect.h);
        }

        for (int y = rect.y; y < rect.y + rect.h; y += dashLen + gapLen)
        {
            SDL_RenderDrawLine(r, rect.x, y, rect.x, std::min(y + dashLen, rect.y + rect.h));
            SDL_RenderDrawLine(r, rect.x + rect.w, y, rect.x + rect.w, std::min(y + dashLen, rect.y + rect.h));
        }
    }

    inline void DrawDirectionalNavArrow(SDL_Renderer *r, const DebugNavDrawRequest &request)
    {
        const SDL_Rect src = request.rect;
        const std::string targetId = ResolveNavTarget(request.widgetId, request.direction);
        if (targetId.empty()) return;

        auto targetIt = g_WidgetRects.find(targetId);
        if (targetIt == g_WidgetRects.end()) return;
        const SDL_Rect dst = targetIt->second;

        int startX = 0, startY = 0, startOutX = 0, startOutY = 0;
        int endOutX = 0, endOutY = 0, endX = 0, endY = 0;

        constexpr int edgeInset = 4;
        constexpr int offset = 10;

        switch (request.direction)
        {
            case NavDir::Up:
                startX = src.x + (src.w / 2);
                startY = src.y + edgeInset;
                startOutX = startX;
                startOutY = src.y - offset;
                endX = dst.x + (dst.w / 2);
                endY = dst.y + dst.h - edgeInset;
                endOutX = endX;
                endOutY = dst.y + dst.h + offset;
                break;
            case NavDir::Down:
                startX = src.x + (src.w / 2);
                startY = src.y + src.h - edgeInset;
                startOutX = startX;
                startOutY = src.y + src.h + offset;
                endX = dst.x + (dst.w / 2);
                endY = dst.y + edgeInset;
                endOutX = endX;
                endOutY = dst.y - offset;
                break;
            case NavDir::Left:
                startX = src.x + edgeInset;
                startY = src.y + (src.h / 2);
                startOutX = src.x - offset;
                startOutY = startY;
                endX = dst.x + dst.w - edgeInset;
                endY = dst.y + (dst.h / 2);
                endOutX = dst.x + dst.w + offset;
                endOutY = endY;
                break;
            case NavDir::Right:
                startX = src.x + src.w - edgeInset;
                startY = src.y + (src.h / 2);
                startOutX = src.x + src.w + offset;
                startOutY = startY;
                endX = dst.x + edgeInset;
                endY = dst.y + (dst.h / 2);
                endOutX = dst.x - offset;
                endOutY = endY;
                break;
            default:
                return;
        }

        if ((startOutX == endOutX) && (startOutY == endOutY))
            return;

        std::vector<SDL_Point> route = {
            {startX, startY},
            {startOutX, startOutY},
            {endOutX, endOutY},
            {endX, endY}
        };

        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, request.color.r, request.color.g, request.color.b, request.color.a);
        for (size_t i = 0; i + 1 < route.size(); ++i)
            SDL_RenderDrawLine(r, route[i].x, route[i].y, route[i + 1].x, route[i + 1].y);

        const SDL_Point &lastStart = route[route.size() - 2];
        const SDL_Point &lastEnd = route[route.size() - 1];
        const float angle = atan2f((float)(lastEnd.y - lastStart.y), (float)(lastEnd.x - lastStart.x));
        constexpr float arrowSpread = (float)M_PI / 7.5f;
        constexpr int arrowLen = 8;

        const int h1x = lastEnd.x - (int)(arrowLen * cosf(angle - arrowSpread));
        const int h1y = lastEnd.y - (int)(arrowLen * sinf(angle - arrowSpread));
        const int h2x = lastEnd.x - (int)(arrowLen * cosf(angle + arrowSpread));
        const int h2y = lastEnd.y - (int)(arrowLen * sinf(angle + arrowSpread));
        SDL_RenderDrawLine(r, lastEnd.x, lastEnd.y, h1x, h1y);
        SDL_RenderDrawLine(r, lastEnd.x, lastEnd.y, h2x, h2y);
    }

    inline void DrawNavRequest(SDL_Renderer *r, const DebugNavDrawRequest &request)
    {
        if (request.kind == DebugNavDrawKind::DirectionalArrow)
        {
            DrawDirectionalNavArrow(r, request);
            return;
        }

        const std::string targetId = ResolveNavTarget(request.widgetId, request.direction);
        if (targetId.empty()) return;

        auto targetIt = g_WidgetRects.find(targetId);
        if (targetIt == g_WidgetRects.end()) return;

        SDL_Rect outline = targetIt->second;
        const int offset = (request.kind == DebugNavDrawKind::NextOutline) ? 3 : 5;
        outline.x -= offset;
        outline.y -= offset;
        outline.w += offset * 2;
        outline.h += offset * 2;

        SDL_Color glow = request.color;
        glow.a = (Uint8)std::min(255, (int)request.color.a / 2);
        SDL_Rect glowRect = outline;
        glowRect.x -= 1;
        glowRect.y -= 1;
        glowRect.w += 2;
        glowRect.h += 2;
        DrawDashedRect(r, glowRect, glow, 8, 4);
        DrawDashedRect(r, outline, request.color, 6, 4);
    }
}

inline void QueueNavDebugRequests(const WidgetDebug &activeDebug, const std::string &widgetId, const SDL_Rect &widgetRect, bool hasActiveFocus)
{
    if (!activeDebug.enabled || !activeDebug.showNavArrows || widgetId.empty())
        return;

    if (activeDebug.focusOnlyNavArrows && !hasActiveFocus)
        return;

    g_DebugNavDrawQueue.push_back({widgetId, NavDir::Up, widgetRect, {255, 80, 80, 170}, DebugNavDrawKind::DirectionalArrow});
    g_DebugNavDrawQueue.push_back({widgetId, NavDir::Down, widgetRect, {80, 255, 80, 170}, DebugNavDrawKind::DirectionalArrow});
    g_DebugNavDrawQueue.push_back({widgetId, NavDir::Left, widgetRect, {80, 180, 255, 170}, DebugNavDrawKind::DirectionalArrow});
    g_DebugNavDrawQueue.push_back({widgetId, NavDir::Right, widgetRect, {255, 180, 50, 170}, DebugNavDrawKind::DirectionalArrow});
    g_DebugNavDrawQueue.push_back({widgetId, NavDir::Next, widgetRect, {0, 240, 255, 210}, DebugNavDrawKind::NextOutline});
    g_DebugNavDrawQueue.push_back({widgetId, NavDir::Prev, widgetRect, {180, 90, 255, 210}, DebugNavDrawKind::PrevOutline});
}

inline void RenderDebugNavigationOverlay(SDL_Renderer *renderer)
{
    if (g_DebugNavDrawQueue.empty())
        return;

    const bool clipEnabled = SDL_RenderIsClipEnabled(renderer);
    SDL_Rect previousClip = {0, 0, 0, 0};
    if (clipEnabled)
        SDL_RenderGetClipRect(renderer, &previousClip);

    SDL_RenderSetClipRect(renderer, nullptr);
    for (const auto &request : g_DebugNavDrawQueue)
        DebugDraw::DrawNavRequest(renderer, request);

    if (clipEnabled)
        SDL_RenderSetClipRect(renderer, &previousClip);
    else
        SDL_RenderSetClipRect(renderer, nullptr);

    g_DebugNavDrawQueue.clear();
}

struct WidgetStateProxy
{
    std::string id;
    void Animate()
    {
        g_UIState[id].isManuallyAnimated = !g_UIState[id].isManuallyAnimated;
        InvalidateUI();
    }
    void Animate(bool state)
    {
        g_UIState[id].isManuallyAnimated = state;
        InvalidateUI();
    }
    bool IsAnimating() const
    {
        return g_UIState[id].isManuallyAnimated;
    }
};

inline WidgetStateProxy GetWidgetById(const std::string &id)
{
    return {id};
}

// Stores the texture cache for a specific widget ID
struct WidgetCache {
    SDL_Texture* texture = nullptr;
    int lastW = 0;
    int lastH = 0;
    int lastX = 0;
    int lastY = 0;
    bool lastHovered = false;
    bool lastFocused = false;
};
inline std::unordered_map<std::string, WidgetCache> g_WidgetCache;

// --- Frame Rendering Hooks for Application Main Loop ---
inline bool BeginUIFrame(SDL_Renderer* renderer, bool forceFullRedraw = false)
{
    const bool animating = HasActiveUIAnimations();
    if (!forceFullRedraw && !g_UINeedsRedraw && !animating)
    {
        return false; // Skip entire frame rendering when idle
    }

    g_UINeedsRedraw = false;
    return true;
}

inline void EndUIFrame(SDL_Renderer* renderer)
{
    ClearDirtyRects();
}

// --- Core Widget Struct ---
struct Widget
{
    std::string id = "";
    Vector2 size = {0, 0};
    WidgetStyle style;
    WidgetDebug debug;
    std::vector<Widget> children; // Must remain 5th element

    std::function<void(SDL_Renderer *, SDL_Rect, const WidgetStyle &, const InputState &, const std::vector<Widget> &, WidgetDebug)> paint; // Must remain 6th
    std::function<void()> onClickFn;
    bool captureFocusOnClick = true;
    bool captureSecondaryFocusOnClick = false;

    float expandX = 0.0f;
    float expandY = 0.0f;

    // Navigation & Keyboard
    WidgetNav nav;
    std::function<void()> onKeyPressFn;
    SDL_Keycode triggerKey = SDLK_RETURN;
    bool enterTriggersClick = true;

    // Text Field Modifiers
    std::string *boundText = nullptr;
    std::function<void(std::string)> onTypeFn;
    std::function<void(std::string)> onValueChangeFn;
    std::function<void(std::string)> onSubmitFn;

    // Parent Reference
    Widget* parent = nullptr;

    bool useCache = false;
    bool disabled = false;

    Widget& WithCache() {
        this->useCache = true;
        return *this;
    }

    Widget& WithDisabled(bool state) {
        this->disabled = state;
        return *this;
    }

    Widget& BindText(std::string *textPtr)
    {
        this->boundText = textPtr;
        return *this;
    }
    Widget& OnType(std::function<void(std::string)> cb)
    {
        this->onTypeFn = cb;
        return *this;
    }
    Widget& OnValueChange(std::function<void(std::string)> cb)
    {
        this->onValueChangeFn = cb;
        return *this;
    }
    Widget& OnSubmit(std::function<void(std::string)> cb)
    {
        this->onSubmitFn = cb;
        return *this;
    }

    Widget& WithScrollbar(ScrollbarStyle sb)
    {
        this->style.scrollbarStyle = sb;
        return *this;
    }

    Widget& WithNav(std::string up, std::string down, std::string left, std::string right, std::string next = "", std::string prev = "")
    {
        this->nav = {std::move(up), std::move(down), std::move(left), std::move(right), std::move(next), std::move(prev)};
        return *this;
    }
    void animatePosition(Vector2 targetPos, float t)
    {
        this->style.position = LerpVector(this->style.position, targetPos, t);
        InvalidateUI();
    }
    void animateSize(Vector2 targetSize, float t) 
    { 
        this->size = LerpVector(this->size, targetSize, t); 
        InvalidateUI();
    }
    void animateColor(SDL_Color targetColor, float t) 
    { 
        this->style.color = LerpColor(this->style.color, targetColor, t); 
        InvalidateUI();
    }
    void animateRadius(int targetRadius, float t) 
    { 
        this->style.radius = (int)(this->style.radius + (targetRadius - this->style.radius) * t); 
        InvalidateUI();
    }
    void animateShadow(SDL_Color targetColor, Vector2 targetOffset, int targetSpread, int targetBlur, float t)
    {
        this->style.shadowColor = LerpColor(this->style.shadowColor, targetColor, t);
        this->style.shadowOffset = LerpVector(this->style.shadowOffset, targetOffset, t);
        this->style.shadowSpread = (int)(this->style.shadowSpread + (targetSpread - this->style.shadowSpread) * t);
        this->style.shadowBlur = (int)(this->style.shadowBlur + (targetBlur - this->style.shadowBlur) * t);
        InvalidateUI();
    }
    void animateBorder(SDL_Color targetColor, int targetWidth, float t)
    {
        this->style.borderColor = LerpColor(this->style.borderColor, targetColor, t);
        this->style.borderWidth = (int)(this->style.borderWidth + (targetWidth - this->style.borderWidth) * t);
        InvalidateUI();
    }

    Widget& OnHover(const std::string &animId, float durationSec, std::function<void(Widget &, float)> animFn, std::function<float(float)> curveFn = Easing::EaseInOutQuad)
    {
        if (this->id.empty()) this->id = animId;
        AnimState &anim = g_UIState[this->id];
        anim.hoverDuration = durationSec;
        anim.curve = curveFn;
        if (anim.hoverProgress > 0.0f)
            animFn(*this, anim.curve(anim.hoverProgress));
        return *this;
    }

    Widget& OnClick(const std::string &animId, float durationSec, std::function<void()> cb, std::function<void(Widget &, float)> animFn, std::function<float(float)> curveFn = Easing::EaseOutQuad, bool captureFocus = true, bool captureSecondaryFocus = false)
    {
        if (this->id.empty()) this->id = animId;
        AnimState &anim = g_UIState[this->id];
        anim.clickDuration = durationSec;
        this->onClickFn = cb;
        this->captureFocusOnClick = captureFocus;
        this->captureSecondaryFocusOnClick = captureSecondaryFocus;
        anim.curve = curveFn;
        if (anim.clickProgress > 0.0f)
            animFn(*this, anim.curve(anim.clickProgress));
        return *this;
    }

    Widget& OnFocus(const std::string &animId, float durationSec, std::function<void(Widget &, float)> animFn, std::function<float(float)> curveFn = Easing::EaseInOutQuad)
    {
        if (this->id.empty()) this->id = animId;
        AnimState &anim = g_UIState[this->id];
        anim.focusDuration = durationSec;
        anim.curve = curveFn;
        if (anim.focusProgress > 0.0f)
            animFn(*this, anim.curve(anim.focusProgress));
        return *this;
    }

    Widget& OnKeyPress(SDL_Keycode key, std::function<void()> cb, bool triggerClickFallback = false)
    {
        this->triggerKey = key;
        this->onKeyPressFn = cb;
        this->enterTriggersClick = triggerClickFallback;
        return *this;
    }

    Widget& OnAnimate(const std::string &animId, float durationSec, std::function<void(Widget &, float)> animFn, std::function<float(float)> curveFn = Easing::EaseInOutQuad)
    {
        if (this->id.empty()) this->id = animId;
        AnimState &anim = g_UIState[this->id];
        anim.manualDuration = durationSec;
        anim.curve = curveFn;
        if (anim.manualProgress > 0.0f)
            animFn(*this, anim.curve(anim.manualProgress));
        return *this;
    }

    WidgetDebug getActiveDebug(WidgetDebug parentDebug) const
    {
        if (g_GlobalDebug.enabled) return g_GlobalDebug;
        if (parentDebug.childrenInherit) return parentDebug;
        return debug;
    }

    void render(SDL_Renderer *renderer, SDL_Rect rect, const InputState &input, WidgetDebug parentDebug = {false}) const
    {
        WidgetStyle effectiveStyle = this->style;
        bool effectiveDisabled = this->disabled;
        const bool hasId = !this->id.empty();

        if (hasId && !g_WidgetInspectorOverrides.empty())
        {
            auto overrideIt = g_WidgetInspectorOverrides.find(this->id);
            if (overrideIt != g_WidgetInspectorOverrides.end())
            {
                const WidgetInspectorOverride &ov = overrideIt->second;
                if (ov.disabledSet) effectiveDisabled = ov.disabled;
                if (ov.colorSet) effectiveStyle.color = ov.color;
                if (ov.positionSet) effectiveStyle.position = ov.position;
                if (ov.radiusSet) effectiveStyle.radius = ov.radius;
            }
        }

        // 0. AUTO-LINK PARENT POINTERS IN-PLACE
        for (auto& child : const_cast<std::vector<Widget>&>(this->children)) {
            child.parent = const_cast<Widget*>(this);
        }

        // 1. CALCULATE RECTS
        SDL_Rect visualRect = rect;
        visualRect.x += (int)effectiveStyle.position.x;
        visualRect.y += (int)effectiveStyle.position.y;

        const SDL_Rect hitbox = effectiveStyle.positionChangesBounds ? visualRect : rect;
        const WidgetDebug activeDebug = this->getActiveDebug(parentDebug);

        bool currentlyHovered = false;
        if (!effectiveDisabled) {
            currentlyHovered = (input.mouseX >= hitbox.x && input.mouseX <= hitbox.x + hitbox.w &&
                                input.mouseY >= hitbox.y && input.mouseY <= hitbox.y + hitbox.h);
        }
        
        bool isFocused = false;

        // 2. STATE UPDATES & DIRTY TRACKING
        if (hasId)
        {
            g_WidgetRects[this->id] = visualRect;
            g_DisabledStates[this->id] = effectiveDisabled;
            g_NavigationLinks[this->id] = this->nav;

            const bool isSecondaryFocused = (g_SecondaryFocusedWidgetId == this->id);
            isFocused = (g_FocusedWidgetId == this->id);
            
            AnimState &anim = g_UIState[this->id];
            const bool justGainedFocus = (!anim.isFocused && isFocused);

            // Invalidate on hover or focus state change
            if (anim.isHovered != currentlyHovered || anim.isFocused != isFocused)
            {
                MarkDirtyRect(visualRect);
            }

            if (g_WidgetInspectorOpen)
            {
                std::string parentId = this->parent ? this->parent->id : "";
                if (parentId.empty() && this->id != "root") parentId = "";
                g_WidgetInspectorRecords.push_back({
                    this->id,
                    parentId,
                    GuessWidgetType(this->id, this->boundText != nullptr, (int)this->children.size()),
                    visualRect,
                    this->size,
                    effectiveStyle.color,
                    effectiveStyle.radius,
                    effectiveStyle.borderWidth,
                    effectiveDisabled,
                    isFocused,
                    currentlyHovered,
                    (int)this->children.size()
                });
            }

            anim.isFocused = isFocused;
            anim.isSecondaryFocused = isSecondaryFocused;
            anim.isHovered = currentlyHovered;

            if (this->boundText != nullptr)
            {
                g_TextFieldBindings[this->id] = this->boundText;
                if (justGainedFocus)
                {
                    g_TextFieldState[this->id].selectionAnchor = g_TextFieldState[this->id].cursorPosition;
                    MarkDirtyRect(visualRect);
                }
            }

            // Mouse Click Handling
            if (!effectiveDisabled && currentlyHovered && input.mouseClicked)
            {
                anim.isClicked = true;
                MarkDirtyRect(visualRect);
                if (this->captureFocusOnClick)
                    g_NextFocusedWidgetId = this->id;
                if (this->captureSecondaryFocusOnClick)
                    g_NextSecondaryFocusedWidgetId = this->id;
                if (this->onClickFn) this->onClickFn();
                
                if (g_Settings.highlightInputCapture)
                {
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 150);
                    SDL_RenderFillRect(renderer, &hitbox);
                }
            }

            // Navigation Handling
            const bool activeMappedNav = !effectiveDisabled && !input.navigationEvents.empty() &&
                                         (isSecondaryFocused || (isFocused && g_SecondaryFocusedWidgetId.empty()));
            if (activeMappedNav)
            {
                auto moveFocus = [&](NavDir direction, const char *label) {
                    if (isSecondaryFocused)
                        MoveSecondaryFocusIfTargetExists(this->id, direction, label);
                    else
                        MoveFocusIfTargetExists(this->id, direction, label);
                };

                for (const NavigationEvent &event : input.navigationEvents)
                {
                    switch (event.action) {
                        case NavAction::Up:
                            moveFocus(NavDir::Up, event.label.c_str());
                            break;
                        case NavAction::Down:
                            moveFocus(NavDir::Down, event.label.c_str());
                            break;
                        case NavAction::Left:
                            if (this->boundText == nullptr || (g_TextFieldState[this->id].cursorPosition == 0 && !(input.keyMod & SDL_KMOD_SHIFT)))
                                moveFocus(NavDir::Left, event.label.c_str());
                            break;
                        case NavAction::Right:
                            if (this->boundText == nullptr || (g_TextFieldState[this->id].cursorPosition == (int)this->boundText->length() && !(input.keyMod & SDL_KMOD_SHIFT)))
                                moveFocus(NavDir::Right, event.label.c_str());
                            break;
                        case NavAction::Prev:
                            moveFocus(NavDir::Prev, event.label.c_str());
                            break;
                        case NavAction::Next:
                            moveFocus(NavDir::Next, event.label.c_str());
                            break;
                    }
                }
            }

            // Key Handling
            const bool activeKeyInput = !effectiveDisabled && input.keyPressed != SDLK_UNKNOWN &&
                                        (isSecondaryFocused || (isFocused && g_SecondaryFocusedWidgetId.empty()));
            if (activeKeyInput && input.keyPressed == this->triggerKey)
            {
                if (this->enterTriggersClick && this->onClickFn)
                {
                    this->onClickFn();
                    anim.isClicked = true;
                    MarkDirtyRect(visualRect);
                }
                else if (this->onKeyPressFn)
                {
                    this->onKeyPressFn();
                    MarkDirtyRect(visualRect);
                }
            }

            // Text Input Handling
            if (isFocused && !effectiveDisabled && this->boundText != nullptr)            
            {
                bool changed = false;
                bool typed = false;
                TextFieldState &tfState = g_TextFieldState[this->id];
                tfState.ClampToText(*this->boundText);
                const bool ctrl = (input.keyMod & SDL_KMOD_CTRL) != 0;
                const bool allowTextNavigation = g_SecondaryFocusedWidgetId.empty();

                if (ctrl && input.keyPressed == SDLK_C)
                {
                    if (tfState.HasSelection())
                    {
                        std::string selection = GetSelectionText(*this->boundText, tfState);
                        SDL_SetClipboardText(selection.c_str());
                    }
                }
                else if (ctrl && input.keyPressed == SDLK_X)
                {
                    if (tfState.HasSelection())
                    {
                        std::string selection = GetSelectionText(*this->boundText, tfState);
                        SDL_SetClipboardText(selection.c_str());
                        tfState.DeleteSelection(*this->boundText);
                        changed = true;
                    }
                }
                else if (ctrl && input.keyPressed == SDLK_V)
                {
                    const char *clipboard = SDL_GetClipboardText();
                    if (clipboard && clipboard[0] != '\0')
                    {
                        if (tfState.HasSelection())
                        {
                            tfState.DeleteSelection(*this->boundText);
                            changed = true;
                        }
                        this->boundText->insert(tfState.cursorPosition, clipboard);
                        tfState.cursorPosition += (int)SDL_strlen(clipboard);
                        tfState.selectionAnchor = tfState.cursorPosition;
                        changed = true;
                        typed = true;
                    }
                }
                else if (ctrl && input.keyPressed == SDLK_A)
                {
                    tfState.selectionAnchor = 0;
                    tfState.cursorPosition = (int)this->boundText->length();
                    MarkDirtyRect(visualRect);
                }

                if (!input.textInput.empty())
                {
                    if (InsertTextIntoTextField(this->id, input.textInput))
                    {
                        changed = true;
                        typed = true;
                    }
                }

                if (input.backspacePressed)
                {
                    if (tfState.HasSelection())
                    {
                        tfState.DeleteSelection(*this->boundText);
                        changed = true;
                    }
                    else if (input.keyMod & SDL_KMOD_CTRL)
                    {
                        int newPos = FindWordBoundaryLeft(*this->boundText, tfState.cursorPosition);
                        if (newPos < tfState.cursorPosition)
                        {
                            this->boundText->erase(newPos, tfState.cursorPosition - newPos);
                            tfState.cursorPosition = newPos;
                            tfState.selectionAnchor = newPos;
                            changed = true;
                        }
                    }
                    else if (tfState.cursorPosition > 0)
                    {
                        this->boundText->erase(tfState.cursorPosition - 1, 1);
                        tfState.cursorPosition--;
                        tfState.selectionAnchor = tfState.cursorPosition;
                        changed = true;
                    }
                }

                if (input.deletePressed)
                {
                    if (input.keyMod & SDL_KMOD_CTRL)
                    {
                        int endPos = tfState.cursorPosition;
                        int targetPos = FindWordBoundaryRight(*this->boundText, tfState.cursorPosition);
                        if (targetPos > endPos)
                        {
                            this->boundText->erase(endPos, targetPos - endPos);
                            changed = true;
                        }
                    }
                    else if (tfState.HasSelection())
                    {
                        tfState.DeleteSelection(*this->boundText);
                        changed = true;
                    }
                    else if (tfState.cursorPosition < (int)this->boundText->length())
                    {
                        this->boundText->erase(tfState.cursorPosition, 1);
                        changed = true;
                    }
                }

                if (allowTextNavigation)
                {
                    if (input.leftPressed)
                    {
                        int targetPos = tfState.cursorPosition;
                        if (input.keyMod & SDL_KMOD_CTRL)
                            targetPos = FindWordBoundaryLeft(*this->boundText, tfState.cursorPosition);
                        else if (tfState.cursorPosition > 0)
                            targetPos = tfState.cursorPosition - 1;

                        if (input.keyMod & SDL_KMOD_SHIFT)
                        {
                            if (!tfState.HasSelection())
                                tfState.selectionAnchor = tfState.cursorPosition;
                            tfState.cursorPosition = targetPos;
                        }
                        else
                        {
                            if (tfState.HasSelection())
                                tfState.cursorPosition = tfState.selectionAnchor = tfState.GetSelectionStart();
                            else
                                tfState.cursorPosition = targetPos;
                            tfState.selectionAnchor = tfState.cursorPosition;
                        }
                        MarkDirtyRect(visualRect);
                    }
                    if (input.rightPressed)
                    {
                        int targetPos = tfState.cursorPosition;
                        if (input.keyMod & SDL_KMOD_CTRL)
                            targetPos = FindWordBoundaryRight(*this->boundText, tfState.cursorPosition);
                        else if (tfState.cursorPosition < (int)this->boundText->length())
                            targetPos = tfState.cursorPosition + 1;

                        if (input.keyMod & SDL_KMOD_SHIFT)
                        {
                            if (!tfState.HasSelection())
                                tfState.selectionAnchor = tfState.cursorPosition;
                            tfState.cursorPosition = targetPos;
                        }
                        else
                        {
                            if (tfState.HasSelection())
                                tfState.cursorPosition = tfState.selectionAnchor = tfState.GetSelectionEnd();
                            else
                                tfState.cursorPosition = targetPos;
                            tfState.selectionAnchor = tfState.cursorPosition;
                        }
                        MarkDirtyRect(visualRect);
                    }
                }

                if (tfState.cursorPosition < 0) tfState.cursorPosition = 0;
                if (tfState.cursorPosition > (int)this->boundText->length()) tfState.cursorPosition = (int)this->boundText->length();

                if (changed || typed) MarkDirtyRect(visualRect);

                if (typed && this->onTypeFn) this->onTypeFn(*this->boundText);
                if (changed && this->onValueChangeFn) this->onValueChangeFn(*this->boundText);
                if (allowTextNavigation && input.enterPressed && this->onSubmitFn) this->onSubmitFn(*this->boundText);
            }
        }

        // 3. PAINT & CACHING
        if (this->useCache && hasId && visualRect.w > 0 && visualRect.h > 0) 
        {
            WidgetCache& cache = g_WidgetCache[this->id];
            bool dirty = false;
            const AnimState &anim = g_UIState[this->id];
            const bool animating = (anim.hoverProgress > 0.0f && anim.hoverProgress < 1.0f) ||
                                   (anim.focusProgress > 0.0f && anim.focusProgress < 1.0f) ||
                                   anim.clickProgress > 0.0f ||
                                   (anim.manualProgress > 0.0f && anim.manualProgress < 1.0f) ||
                                   anim.isClicked || anim.isManuallyAnimated;
            const bool focusedTextEditing = isFocused && this->boundText != nullptr &&
                                           (!input.textInput.empty() || input.backspacePressed || input.deletePressed ||
                                            input.leftPressed || input.rightPressed || input.enterPressed);

            if (cache.lastW != visualRect.w || cache.lastH != visualRect.h || !cache.texture) 
            {
                if (cache.texture) SDL_DestroyTexture(cache.texture);
                cache.texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, 
                                                SDL_TEXTUREACCESS_TARGET, visualRect.w, visualRect.h);
                if (cache.texture)
                    SDL_SetTextureBlendMode(cache.texture, SDL_BLENDMODE_BLEND);
                dirty = true;
            }

            if (cache.lastHovered != currentlyHovered || cache.lastFocused != isFocused || animating || focusedTextEditing)
            {
                dirty = true;
            }

            if ((dirty || !cache.texture) && cache.texture) 
            {
                SDL_Texture* prevTarget = SDL_GetRenderTarget(renderer);
                SDL_SetRenderTarget(renderer, cache.texture);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
                SDL_RenderClear(renderer);

                SDL_Rect localRect = {0, 0, visualRect.w, visualRect.h};
                if (this->paint) this->paint(renderer, localRect, effectiveStyle, input, this->children, activeDebug);

                SDL_SetRenderTarget(renderer, prevTarget);

                cache.lastW = visualRect.w;
                cache.lastH = visualRect.h;
                cache.lastX = visualRect.x;
                cache.lastY = visualRect.y;
                cache.lastHovered = currentlyHovered;
                cache.lastFocused = isFocused;
            }

            if (cache.texture)
                SDL_RenderCopy(renderer, cache.texture, nullptr, &visualRect);
        }
        else 
        {
            if (this->paint)
                this->paint(renderer, visualRect, effectiveStyle, input, this->children, activeDebug);
        }

        // 4. DEBUG & METRICS
        if (g_Settings.paintFlashMode)
        {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, rand() % 255, rand() % 255, rand() % 255, 40);
            SDL_RenderFillRect(renderer, &visualRect);
        }

        if (g_Settings.showClipRects && SDL_RenderIsClipEnabled(renderer))
        {
            SDL_Rect clip;
            SDL_RenderGetClipRect(renderer, &clip);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 255, 128, 0, 200);
            SDL_RenderDrawRect(renderer, &clip);
            SDL_Rect innerClip = {clip.x + 1, clip.y + 1, clip.w - 2, clip.h - 2};
            SDL_RenderDrawRect(renderer, &innerClip);
        }

        if (activeDebug.enabled)
        {
            if (activeDebug.showBounds)
            {
                DebugDraw::DrawBoundsCross(renderer, visualRect, activeDebug.boundsColor);
                if (!effectiveStyle.positionChangesBounds)
                {
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 120);
                    SDL_RenderDrawRect(renderer, &hitbox);
                }
            }

            QueueNavDebugRequests(activeDebug, this->id, visualRect, isFocused || g_SecondaryFocusedWidgetId == this->id);
        }
    }
};