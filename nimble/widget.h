#pragma once
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <unordered_set>
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

inline std::string g_FocusedWidgetId = "";
inline std::string g_NextFocusedWidgetId = "";
inline std::string g_SecondaryFocusedWidgetId = "";
inline std::string g_NextSecondaryFocusedWidgetId = "";
inline bool g_inputFocused = false;
inline std::unordered_map<std::string, SDL_Rect> g_WidgetRects;
inline bool g_DebugNavigation = true;
inline std::unordered_map<std::string, bool> g_DisabledStates;
    
// --- Navigation Struct ---
struct WidgetNav
{
    std::string up = "";
    std::string down = "";
    std::string left = "";
    std::string right = "";
    std::string next = ""; // Tab
    std::string prev = ""; // Shift + Tab
};

inline std::unordered_map<std::string, WidgetNav> g_NavigationLinks;

inline void NavLog(const std::string &from,
                   const std::string &dir,
                   const std::string &to)
{
    if (!g_DebugNavigation)
        return;

    std::cout << "[NAV] "
              << from
              << " -> " << dir
              << " -> " << (to.empty() ? "NONE" : to)
              << std::endl;
}

inline std::string ResolveNavTarget(const std::string& startId, const std::string& direction)
{
    auto it = g_NavigationLinks.find(startId);
    if (it == g_NavigationLinks.end()) return "";

    std::string currentTarget = "";
    if (direction == "up")    currentTarget = it->second.up;
    else if (direction == "down")  currentTarget = it->second.down;
    else if (direction == "left")  currentTarget = it->second.left;
    else if (direction == "right") currentTarget = it->second.right;
    else if (direction == "next")  currentTarget = it->second.next;
    else if (direction == "prev")  currentTarget = it->second.prev;

    std::unordered_set<std::string> visited;

    // While the target is disabled, "hop" to that target's own neighbor in the same direction
    while (!currentTarget.empty() && g_DisabledStates[currentTarget])
    {
        if (visited.count(currentTarget)) return ""; // Prevent infinite loops
        visited.insert(currentTarget);

        auto nextIt = g_NavigationLinks.find(currentTarget);
        if (nextIt == g_NavigationLinks.end()) break;

        if (direction == "up")    currentTarget = nextIt->second.up;
        else if (direction == "down")  currentTarget = nextIt->second.down;
        else if (direction == "left")  currentTarget = nextIt->second.left;
        else if (direction == "right") currentTarget = nextIt->second.right;
        else if (direction == "next")  currentTarget = nextIt->second.next;
        else if (direction == "prev")  currentTarget = nextIt->second.prev;
    }

    return currentTarget;
}

inline void MoveFocusIfTargetExists(const std::string &fromId, const char *direction, const char *label)
{
    const std::string target = ResolveNavTarget(fromId, direction);
    if (!target.empty())
        g_NextFocusedWidgetId = target;
    NavLog(fromId, label, target);
}

// --- Debugging Structures ---
struct WidgetDebug
{
    bool enabled = true;
    bool showBounds = false;
    bool showPadding = false;
    bool showSpacing = false;
    bool showExpanded = false;
    bool showRow = false;
    bool showColumn = false;
    bool showNavArrows = true;
    bool focusOnlyNavArrows = true;
    bool childrenInherit = false;

    SDL_Color boundsColor = {255, 0, 0, 255};      // Red cross
    SDL_Color paddingColor = {170, 255, 170, 120}; // Pastel Green
    SDL_Color spacingColor = {255, 0, 255, 150};   // Magenta
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
    std::string direction;
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
        if (targetId.empty())
            return;

        auto targetIt = g_WidgetRects.find(targetId);
        if (targetIt == g_WidgetRects.end())
            return;
        const SDL_Rect dst = targetIt->second;
        const std::string &dirName = request.direction;

        int startX = 0, startY = 0;
        int startOutX = 0, startOutY = 0;
        int endOutX = 0, endOutY = 0;
        int endX = 0, endY = 0;

        constexpr int edgeInset = 4;
        constexpr int offset = 10;

        if (dirName == "up")
        {
            startX = src.x + (src.w / 2);
            startY = src.y + edgeInset;
            startOutX = startX;
            startOutY = src.y - offset;
            endX = dst.x + (dst.w / 2);
            endY = dst.y + dst.h - edgeInset;
            endOutX = endX;
            endOutY = dst.y + dst.h + offset;
        }
        else if (dirName == "down")
        {
            startX = src.x + (src.w / 2);
            startY = src.y + src.h - edgeInset;
            startOutX = startX;
            startOutY = src.y + src.h + offset;
            endX = dst.x + (dst.w / 2);
            endY = dst.y + edgeInset;
            endOutX = endX;
            endOutY = dst.y - offset;
        }
        else if (dirName == "left")
        {
            startX = src.x + edgeInset;
            startY = src.y + (src.h / 2);
            startOutX = src.x - offset;
            startOutY = startY;
            endX = dst.x + dst.w - edgeInset;
            endY = dst.y + (dst.h / 2);
            endOutX = dst.x + dst.w + offset;
            endOutY = endY;
        }
        else if (dirName == "right")
        {
            startX = src.x + src.w - edgeInset;
            startY = src.y + (src.h / 2);
            startOutX = src.x + src.w + offset;
            startOutY = startY;
            endX = dst.x + edgeInset;
            endY = dst.y + (dst.h / 2);
            endOutX = dst.x - offset;
            endOutY = endY;
        }
        else
        {
            return;
        }

        if ((startOutX == endOutX) && (startOutY == endOutY))
            return;

        auto lineIntersectsRect = [](int x1, int y1, int x2, int y2, const SDL_Rect &rect)
        {
            int lx1 = x1, ly1 = y1, lx2 = x2, ly2 = y2;
            return SDL_IntersectRectAndLine(&rect, &lx1, &ly1, &lx2, &ly2) == SDL_TRUE;
        };

        auto countPathCollisions = [&](const std::vector<SDL_Point> &pts)
        {
            if (pts.size() < 2)
                return 0;

            int collisions = 0;
            for (const auto &entry : g_WidgetRects)
            {
                if (entry.first == request.widgetId || entry.first == targetId)
                    continue;

                SDL_Rect obstacle = entry.second;
                obstacle.x -= 2;
                obstacle.y -= 2;
                obstacle.w += 4;
                obstacle.h += 4;
                for (size_t i = 0; i + 1 < pts.size(); ++i)
                {
                    if (lineIntersectsRect(pts[i].x, pts[i].y, pts[i + 1].x, pts[i + 1].y, obstacle))
                    {
                        collisions++;
                        break;
                    }
                }
            }
            return collisions;
        };

        auto routeLength = [](const std::vector<SDL_Point> &pts)
        {
            int length = 0;
            for (size_t i = 0; i + 1 < pts.size(); ++i)
                length += std::abs(pts[i + 1].x - pts[i].x) + std::abs(pts[i + 1].y - pts[i].y);
            return length;
        };

        std::vector<SDL_Point> route = {
            {startX, startY},
            {startOutX, startOutY},
            {endOutX, endOutY},
            {endX, endY}
        };

        const int routeMargin = 16;
        std::vector<std::vector<SDL_Point>> candidates;
        candidates.push_back(route);

        if (dirName == "left" || dirName == "right")
        {
            std::vector<int> yLanes;
            yLanes.push_back(std::max(src.y + src.h, dst.y + dst.h) + routeMargin); // Prefer under
            yLanes.push_back(std::min(src.y, dst.y) - routeMargin);                  // Then above

            for (const auto &entry : g_WidgetRects)
            {
                if (entry.first == request.widgetId || entry.first == targetId)
                    continue;
                const SDL_Rect &o = entry.second;
                yLanes.push_back(o.y - routeMargin);
                yLanes.push_back(o.y + o.h + routeMargin);
            }

            std::sort(yLanes.begin(), yLanes.end());
            yLanes.erase(std::unique(yLanes.begin(), yLanes.end()), yLanes.end());

            for (int y : yLanes)
            {
                candidates.push_back({
                    {startX, startY},
                    {startOutX, startOutY},
                    {startOutX, y},
                    {endOutX, y},
                    {endOutX, endOutY},
                    {endX, endY}
                });
            }
        }
        else if (dirName == "up" || dirName == "down")
        {
            std::vector<int> xLanes;
            xLanes.push_back(std::max(src.x + src.w, dst.x + dst.w) + routeMargin);
            xLanes.push_back(std::min(src.x, dst.x) - routeMargin);

            for (const auto &entry : g_WidgetRects)
            {
                if (entry.first == request.widgetId || entry.first == targetId)
                    continue;
                const SDL_Rect &o = entry.second;
                xLanes.push_back(o.x - routeMargin);
                xLanes.push_back(o.x + o.w + routeMargin);
            }

            std::sort(xLanes.begin(), xLanes.end());
            xLanes.erase(std::unique(xLanes.begin(), xLanes.end()), xLanes.end());

            for (int x : xLanes)
            {
                candidates.push_back({
                    {startX, startY},
                    {startOutX, startOutY},
                    {x, startOutY},
                    {x, endOutY},
                    {endOutX, endOutY},
                    {endX, endY}
                });
            }
        }

        int bestScore = INT_MAX;
        std::vector<SDL_Point> bestRoute = route;
        for (const auto &candidate : candidates)
        {
            const int collisions = countPathCollisions(candidate);
            const int length = routeLength(candidate);
            // Strongly prioritize no-collision routes; then prefer shorter.
            const int score = collisions * 100000 + length;
            if (score < bestScore)
            {
                bestScore = score;
                bestRoute = candidate;
            }
        }
        route = bestRoute;

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
        if (targetId.empty())
            return;

        auto targetIt = g_WidgetRects.find(targetId);
        if (targetIt == g_WidgetRects.end())
            return;

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

inline void QueueNavDebugRequests(const WidgetDebug &activeDebug, const std::string &widgetId, const SDL_Rect &widgetRect, bool isFocused)
{
    if (!activeDebug.enabled || !activeDebug.showNavArrows || widgetId.empty())
        return;

    if (activeDebug.focusOnlyNavArrows && !isFocused)
        return;

    auto queueDirection = [&](const std::string &direction, SDL_Color color)
    {
        g_DebugNavDrawQueue.push_back({
            widgetId,
            direction,
            widgetRect,
            color,
            DebugNavDrawKind::DirectionalArrow
        });
    };

    auto queueOutline = [&](const std::string &direction, SDL_Color color, DebugNavDrawKind kind)
    {
        g_DebugNavDrawQueue.push_back({
            widgetId,
            direction,
            widgetRect,
            color,
            kind
        });
    };

    queueDirection("up", {255, 80, 80, 170});
    queueDirection("down", {80, 255, 80, 170});
    queueDirection("left", {80, 180, 255, 170});
    queueDirection("right", {255, 180, 50, 170});

    queueOutline("next", {0, 240, 255, 210}, DebugNavDrawKind::NextOutline);
    queueOutline("prev", {180, 90, 255, 210}, DebugNavDrawKind::PrevOutline);
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
    }
    void Animate(bool state)
    {
        g_UIState[id].isManuallyAnimated = state;
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

// --- Core Widget Struct ---
struct Widget
{
    std::string id = "";
    Vector2 size = {0, 0};
    WidgetStyle style;
    WidgetDebug debug;
    std::vector<Widget> children; // Must remain the 5th element!

    std::function<void(SDL_Renderer *, SDL_Rect, const WidgetStyle &, const InputState &, const std::vector<Widget> &, WidgetDebug)> paint; // Must remain 6th!
    std::function<void()> onClickFn;
    bool captureFocusOnClick = true;
    bool captureSecondaryFocusOnClick = false;

    // --- Placed at the end to not break positional {} initializations ---
    float expandX = 0.0f;
    float expandY = 0.0f;

    // Navigation & Keyboard
    WidgetNav nav;
    std::function<void()> onKeyPressFn;
    SDL_Keycode triggerKey = SDLK_RETURN; // Default Enter key
    bool enterTriggersClick = true;

    // --- Text Field Modifiers ---
    std::string *boundText = nullptr;
    std::function<void(std::string)> onTypeFn;
    std::function<void(std::string)> onValueChangeFn;
    std::function<void(std::string)> onSubmitFn;

    // --- Parent Reference ---
    Widget* parent = nullptr;

    bool useCache = false; // Add this

    bool disabled = false; // New field

    Widget WithCache() {
        this->useCache = true;
        return *this;
    }

    Widget WithDisabled(bool state) {
        this->disabled = state;
        return *this;
    }


    Widget BindText(std::string *textPtr)
    {
        this->boundText = textPtr;
        return *this;
    }
    Widget OnType(std::function<void(std::string)> cb)
    {
        this->onTypeFn = cb;
        return *this;
    }
    Widget OnValueChange(std::function<void(std::string)> cb)
    {
        this->onValueChangeFn = cb;
        return *this;
    }
    Widget OnSubmit(std::function<void(std::string)> cb)
    {
        this->onSubmitFn = cb;
        return *this;
    }

    // Chain modifiers
    Widget WithScrollbar(ScrollbarStyle sb)
    {
        this->style.scrollbarStyle = sb;
        return *this;
    }

    // --- Navigation Modifiers ---
    Widget WithNav(std::string up, std::string down, std::string left, std::string right, std::string next = "", std::string prev = "")
    {
        this->nav = {up, down, left, right, next, prev};
        return *this;
    }
    void animatePosition(Vector2 targetPos, float t)
    {
        this->style.position = LerpVector(this->style.position, targetPos, t);
    }
    void animateSize(Vector2 targetSize, float t) { this->size = LerpVector(this->size, targetSize, t); }
    void animateColor(SDL_Color targetColor, float t) { this->style.color = LerpColor(this->style.color, targetColor, t); }
    void animateRadius(int targetRadius, float t) { this->style.radius = (int)(this->style.radius + (targetRadius - this->style.radius) * t); }
    void animateShadow(SDL_Color targetColor, Vector2 targetOffset, int targetSpread, int targetBlur, float t)
    {
        this->style.shadowColor = LerpColor(this->style.shadowColor, targetColor, t);
        this->style.shadowOffset = LerpVector(this->style.shadowOffset, targetOffset, t);
        this->style.shadowSpread = (int)(this->style.shadowSpread + (targetSpread - this->style.shadowSpread) * t);
        this->style.shadowBlur = (int)(this->style.shadowBlur + (targetBlur - this->style.shadowBlur) * t);
    }
    void animateBorder(SDL_Color targetColor, int targetWidth, float t)
    {
        this->style.borderColor = LerpColor(this->style.borderColor, targetColor, t);
        this->style.borderWidth = (int)(this->style.borderWidth + (targetWidth - this->style.borderWidth) * t);
    }

    Widget OnHover(const std::string &animId, float durationSec, std::function<void(Widget &, float)> animFn, std::function<float(float)> curveFn = Easing::EaseInOutQuad)
    {
        if (this->id.empty())
            this->id = animId;
        AnimState &anim = g_UIState[this->id];
        anim.hoverDuration = durationSec;
        anim.curve = curveFn;
        if (anim.hoverProgress > 0.0f)
            animFn(*this, anim.curve(anim.hoverProgress));
        return *this;
    }

    Widget OnClick(const std::string &animId, float durationSec, std::function<void()> cb, std::function<void(Widget &, float)> animFn, std::function<float(float)> curveFn = Easing::EaseOutQuad, bool captureFocus = true, bool captureSecondaryFocus = false)
    {
        if (this->id.empty())
            this->id = animId;
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

    Widget OnFocus(const std::string &animId, float durationSec, std::function<void(Widget &, float)> animFn, std::function<float(float)> curveFn = Easing::EaseInOutQuad)
    {
        if (this->id.empty())
            this->id = animId;
        AnimState &anim = g_UIState[this->id];
        anim.focusDuration = durationSec;
        anim.curve = curveFn; // (Make sure focusDuration & focusProgress are added to AnimState)
        if (anim.focusProgress > 0.0f)
            animFn(*this, anim.curve(anim.focusProgress));
        return *this;
    }

    Widget OnKeyPress(SDL_Keycode key, std::function<void()> cb, bool triggerClickFallback = false)
    {
        this->triggerKey = key;
        this->onKeyPressFn = cb;
        this->enterTriggersClick = triggerClickFallback;
        return *this;
    }

    Widget OnAnimate(const std::string &animId, float durationSec, std::function<void(Widget &, float)> animFn, std::function<float(float)> curveFn = Easing::EaseInOutQuad)
    {
        if (this->id.empty())
            this->id = animId;
        AnimState &anim = g_UIState[this->id];
        anim.manualDuration = durationSec;
        anim.curve = curveFn;
        if (anim.manualProgress > 0.0f)
            animFn(*this, anim.curve(anim.manualProgress));
        return *this;
    }

    WidgetDebug getActiveDebug(WidgetDebug parentDebug) const
    {
        if (g_GlobalDebug.enabled)
            return g_GlobalDebug;
        if (parentDebug.childrenInherit)
            return parentDebug;
        return debug;
    }

    void render(SDL_Renderer *renderer, SDL_Rect rect, const InputState &input, WidgetDebug parentDebug = {false}) const
    {
        // 0. AUTO-LINK PARENT POINTERS
        // This guarantees `child.parent` is safely pointing to the actual layout memory during this frame
        for (auto& child : const_cast<std::vector<Widget>&>(children)) {
            child.parent = const_cast<Widget*>(this);
        }

        // 1. CALCULATE RECTS (Position vs. Hitbox)
        // visualRect is where it's drawn.
        // hitbox is where the mouse interacts.
        SDL_Rect visualRect = rect;
        visualRect.x += (int)style.position.x;
        visualRect.y += (int)style.position.y;

        SDL_Rect hitbox = style.positionChangesBounds ? visualRect : rect;

        WidgetDebug activeDebug = getActiveDebug(parentDebug);
        bool currentlyHovered = false;
        if (!this->disabled) {
            currentlyHovered = (input.mouseX >= hitbox.x && input.mouseX <= hitbox.x + hitbox.w &&
                                input.mouseY >= hitbox.y && input.mouseY <= hitbox.y + hitbox.h);
        }
        
        bool isFocused = false;

        // 2. STATE UPDATES & INTERACTION
        if (!this->id.empty())
        {
            // Save visual position for debug arrow rendering
            g_WidgetRects[this->id] = visualRect;
            g_DisabledStates[this->id] = this->disabled;       // Store disabled state
            g_NavigationLinks[this->id] = this->nav;           // Store nav links

            bool isSecondaryFocused = (g_SecondaryFocusedWidgetId == this->id);
            bool justGainedFocus = (!g_UIState[this->id].isFocused && (g_FocusedWidgetId == this->id));
            isFocused = (g_FocusedWidgetId == this->id);

            // Sync global state for animations (Progress is updated in UpdateUIAnimations)
            g_UIState[this->id].isFocused = isFocused;
            g_UIState[this->id].isSecondaryFocused = isSecondaryFocused;
            g_UIState[this->id].isHovered = currentlyHovered;

            if (justGainedFocus && boundText != nullptr)
                g_TextFieldState[this->id].selectionAnchor = g_TextFieldState[this->id].cursorPosition;

            // Handle Mouse Click & Focus Gain
            
            if (!this->disabled && currentlyHovered && input.mouseClicked)
            {
                g_UIState[this->id].isClicked = true;
                if (this->captureFocusOnClick)
                    g_NextFocusedWidgetId = this->id;
                if (this->captureSecondaryFocusOnClick)
                    g_NextSecondaryFocusedWidgetId = this->id;
                if (onClickFn) onClickFn();                // --- 2.2 Input Capture Highlight ---
                if (g_Settings.highlightInputCapture)
                {
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 150); // Flash Green
                    SDL_RenderFillRect(renderer, &hitbox);
                }
            }

            // 3. KEYBOARD NAVIGATION HANDLING
            bool activeKeyboardNav = !this->disabled && input.keyPressed != SDLK_UNKNOWN &&
                                     (isSecondaryFocused || (isFocused && g_SecondaryFocusedWidgetId.empty()));
            if (activeKeyboardNav)
            {
                if (input.keyPressed == SDLK_UP) {
                    MoveFocusIfTargetExists(this->id, "up", "UP");
                }
                else if (input.keyPressed == SDLK_DOWN) {
                    MoveFocusIfTargetExists(this->id, "down", "DOWN");
                }
                else if (input.keyPressed == SDLK_LEFT) {
                    if (boundText == nullptr || (g_TextFieldState[this->id].cursorPosition == 0 && !(input.keyMod & KMOD_SHIFT))) {
                        MoveFocusIfTargetExists(this->id, "left", "LEFT");
                    }
                }
                else if (input.keyPressed == SDLK_RIGHT) {
                    if (boundText == nullptr || (g_TextFieldState[this->id].cursorPosition == (int)boundText->length() && !(input.keyMod & KMOD_SHIFT))) {
                        MoveFocusIfTargetExists(this->id, "right", "RIGHT");
                    }
                }
                else if (input.keyPressed == SDLK_TAB) {
                    if (input.keyMod & KMOD_SHIFT) {
                        MoveFocusIfTargetExists(this->id, "prev", "SHIFT+TAB");
                    } else {
                        MoveFocusIfTargetExists(this->id, "next", "TAB");
                    }
                }
                // Trigger Action (Enter key)
                if (input.keyPressed == this->triggerKey)
                {
                    if (this->enterTriggersClick && this->onClickFn)
                    {
                        this->onClickFn();
                        g_UIState[this->id].isClicked = true;
                    }
                    else if (this->onKeyPressFn)
                    {
                        this->onKeyPressFn();
                    }
                }
            }

            // 4. TEXT INPUT LOGIC (TextField handling)
            if (isFocused && !this->disabled && boundText != nullptr)            
            {
                bool changed = false;
                bool typed = false;
                TextFieldState &tfState = g_TextFieldState[this->id];
                bool ctrl = (input.keyMod & KMOD_CTRL) != 0;
                bool allowTextNavigation = g_SecondaryFocusedWidgetId.empty();

                // Clipboard shortcuts
                if (ctrl && input.keyPressed == SDLK_c)
                {
                    if (tfState.HasSelection())
                    {
                        std::string selection = GetSelectionText(*boundText, tfState);
                        SDL_SetClipboardText(selection.c_str());
                    }
                }
                else if (ctrl && input.keyPressed == SDLK_x)
                {
                    if (tfState.HasSelection())
                    {
                        std::string selection = GetSelectionText(*boundText, tfState);
                        SDL_SetClipboardText(selection.c_str());
                        tfState.DeleteSelection(*boundText);
                        changed = true;
                    }
                }
                else if (ctrl && input.keyPressed == SDLK_v)
                {
                    const char *clipboard = SDL_GetClipboardText();
                    if (clipboard && clipboard[0] != '\0')
                    {
                        if (tfState.HasSelection())
                        {
                            tfState.DeleteSelection(*boundText);
                            changed = true;
                        }
                        boundText->insert(tfState.cursorPosition, clipboard);
                        tfState.cursorPosition += (int)SDL_strlen(clipboard);
                        tfState.selectionAnchor = tfState.cursorPosition;
                        changed = true;
                        typed = true;
                    }
                }
                else if (ctrl && input.keyPressed == SDLK_a)
                {
                    tfState.selectionAnchor = 0;
                    tfState.cursorPosition = (int)boundText->length();
                }

                // Character Insertion
                if (!input.textInput.empty())
                {
                    if (tfState.HasSelection())
                    {
                        tfState.DeleteSelection(*boundText);
                        changed = true;
                    }
                    boundText->insert(tfState.cursorPosition, input.textInput);
                    tfState.cursorPosition += (int)input.textInput.length();
                    tfState.selectionAnchor = tfState.cursorPosition;
                    changed = true;
                    typed = true;
                }
                // Backspace
                if (input.backspacePressed)
                {
                    if (tfState.HasSelection())
                    {
                        tfState.DeleteSelection(*boundText);
                        changed = true;
                    }
                    else if (input.keyMod & KMOD_CTRL)
                    {
                        int newPos = FindWordBoundaryLeft(*boundText, tfState.cursorPosition);
                        if (newPos < tfState.cursorPosition)
                        {
                            boundText->erase(newPos, tfState.cursorPosition - newPos);
                            tfState.cursorPosition = newPos;
                            tfState.selectionAnchor = newPos;
                            changed = true;
                        }
                    }
                    else if (tfState.cursorPosition > 0)
                    {
                        boundText->erase(tfState.cursorPosition - 1, 1);
                        tfState.cursorPosition--;
                        tfState.selectionAnchor = tfState.cursorPosition;
                        changed = true;
                    }
                }
                // Delete key
                if (input.deletePressed)
                {
                    if (input.keyMod & KMOD_CTRL)
                    {
                        int endPos = tfState.cursorPosition;
                        int targetPos = FindWordBoundaryRight(*boundText, tfState.cursorPosition);
                        if (targetPos > endPos)
                        {
                            boundText->erase(endPos, targetPos - endPos);
                            changed = true;
                        }
                    }
                    else if (tfState.HasSelection())
                    {
                        tfState.DeleteSelection(*boundText);
                        changed = true;
                    }
                    else if (tfState.cursorPosition < (int)boundText->length())
                    {
                        boundText->erase(tfState.cursorPosition, 1);
                        changed = true;
                    }
                }
                // Cursor Movement
                if (allowTextNavigation)
                {
                    if (input.leftPressed)
                    {
                        int targetPos = tfState.cursorPosition;
                        if (input.keyMod & KMOD_CTRL)
                        {
                            targetPos = FindWordBoundaryLeft(*boundText, tfState.cursorPosition);
                        }
                        else if (tfState.cursorPosition > 0)
                        {
                            targetPos = tfState.cursorPosition - 1;
                        }

                        if (input.keyMod & KMOD_SHIFT)
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
                    }
                    if (input.rightPressed)
                    {
                        int targetPos = tfState.cursorPosition;
                        if (input.keyMod & KMOD_CTRL)
                        {
                            targetPos = FindWordBoundaryRight(*boundText, tfState.cursorPosition);
                        }
                        else if (tfState.cursorPosition < (int)boundText->length())
                        {
                            targetPos = tfState.cursorPosition + 1;
                        }

                        if (input.keyMod & KMOD_SHIFT)
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
                    }
                }

                // Bounds clamping for safety
                if (tfState.cursorPosition < 0)
                    tfState.cursorPosition = 0;
                if (tfState.cursorPosition > (int)boundText->length())
                    tfState.cursorPosition = (int)boundText->length();

                // Fire Events
                if (typed && onTypeFn)
                    onTypeFn(*boundText);
                if (changed && onValueChangeFn)
                    onValueChangeFn(*boundText);
                if (allowTextNavigation && input.enterPressed && onSubmitFn)
                    onSubmitFn(*boundText);
            }
        }

        // 5. PAINT & CACHE
        if (this->useCache && !this->id.empty()) 
        {
            WidgetCache& cache = g_WidgetCache[this->id];
            bool dirty = false;
            const AnimState &anim = g_UIState[this->id];
            const bool animating = (anim.hoverProgress > 0.0f && anim.hoverProgress < 1.0f) ||
                                   (anim.focusProgress > 0.0f && anim.focusProgress < 1.0f) ||
                                   anim.clickProgress > 0.0f ||
                                   (anim.manualProgress > 0.0f && anim.manualProgress < 1.0f) ||
                                   anim.isClicked || anim.isManuallyAnimated;
            const bool focusedTextEditing = isFocused && boundText != nullptr &&
                                           (!input.textInput.empty() || input.backspacePressed || input.deletePressed ||
                                            input.leftPressed || input.rightPressed || input.enterPressed);

            // Determine if the widget's visuals need to be refreshed
            if (cache.lastW != visualRect.w || cache.lastH != visualRect.h) dirty = true;
            if (cache.lastHovered != currentlyHovered) dirty = true;
            if (cache.lastFocused != isFocused) dirty = true;
            if (cache.lastX != visualRect.x || cache.lastY != visualRect.y) dirty = true;
            if (animating) dirty = true;
            if (focusedTextEditing) dirty = true;

            // If state changed OR texture doesn't exist, draw to texture
            if (dirty || !cache.texture) 
            {
                if (cache.texture) SDL_DestroyTexture(cache.texture);
                cache.texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, 
                                                SDL_TEXTUREACCESS_TARGET, visualRect.w, visualRect.h);
                SDL_SetTextureBlendMode(cache.texture, SDL_BLENDMODE_BLEND);

                // Redirect rendering to our cache texture
                SDL_SetRenderTarget(renderer, cache.texture);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0); // Transparent background
                SDL_RenderClear(renderer);

                // Call the widget's paint function starting at local coordinates {0, 0}
                SDL_Rect localRect = {0, 0, visualRect.w, visualRect.h};
                if (paint) paint(renderer, localRect, style, input, children, activeDebug);

                // Restore rendering back to the screen
                SDL_SetRenderTarget(renderer, nullptr);

                // Update cached states
                cache.lastW = visualRect.w;
                cache.lastH = visualRect.h;
                cache.lastX = visualRect.x;
                cache.lastY = visualRect.y;
                cache.lastHovered = currentlyHovered;
                cache.lastFocused = isFocused;
            }

            // Draw the cached texture to the screen
            SDL_RenderCopy(renderer, cache.texture, nullptr, &visualRect);
        }
        else 
        {
            // Normal immediate rendering (No Cache)
            if (paint)
                paint(renderer, visualRect, style, input, children, activeDebug);
        }
        // --- 3.2 Paint-Flash Mode ---
        if (g_Settings.paintFlashMode)
        {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, rand() % 255, rand() % 255, rand() % 255, 40); // Random tint flash per render call
            SDL_RenderFillRect(renderer, &visualRect);
        }

        // --- 1.2 Clip Rect Visualization ---
        if (g_Settings.showClipRects)
        {
            SDL_Rect clip;
            if (SDL_RenderIsClipEnabled(renderer))
            {
                SDL_RenderGetClipRect(renderer, &clip);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 255, 128, 0, 200); // Bright orange dashed clip bounds
                SDL_RenderDrawRect(renderer, &clip);
                
                // Draw an inner rect to make it thicker and easier to see
                SDL_Rect innerClip = {clip.x + 1, clip.y + 1, clip.w - 2, clip.h - 2};
                SDL_RenderDrawRect(renderer, &innerClip);
            }
        }

        // 6. DEBUG OVERLAYS
        if (activeDebug.enabled)
        {
            if (activeDebug.showBounds)
            {
                // Draw visual bounds (Where it is rendered)
                DebugDraw::DrawBoundsCross(renderer, visualRect, activeDebug.boundsColor);

                // If the hitbox is different from visual (positionChangesBounds = false), draw hitbox in Cyan
                if (!style.positionChangesBounds)
                {
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 120);
                    SDL_RenderDrawRect(renderer, &hitbox);
                }
            }

            QueueNavDebugRequests(activeDebug, this->id, visualRect, isFocused);
        }
    }
};