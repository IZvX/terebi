#pragma once
#include <cmath>
#include <algorithm>

inline std::string g_FocusedWidgetId = "";
inline std::string g_NextFocusedWidgetId = "";
inline std::unordered_map<std::string, SDL_Rect> g_WidgetRects;
inline bool g_DebugNavigation = true;

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

// --- Debugging Structures ---
struct WidgetDebug
{
    bool enabled = false;
    bool showBounds = false;
    bool showPadding = false;
    bool showSpacing = false;
    bool showNavArrows = false;
    bool childrenInherit = false;

    SDL_Color boundsColor = {255, 0, 0, 255};      // Red cross
    SDL_Color paddingColor = {170, 255, 170, 120}; // Pastel Green
    SDL_Color spacingColor = {255, 0, 255, 150};   // Magenta
};

inline WidgetDebug g_GlobalDebug = {false, false, false, false, false};


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

    // --- DEAD SIMPLE EDGE-TO-EDGE ARROW ---
    inline void DrawEdgeToEdgeArrow(SDL_Renderer *r, SDL_Rect src, SDL_Rect dst, SDL_Color color)
    {
        // 1. Find the centers of both widgets
        int srcCx = src.x + src.w / 2;
        int srcCy = src.y + src.h / 2;
        int dstCx = dst.x + dst.w / 2;
        int dstCy = dst.y + dst.h / 2;

        // 2. Clamp target's center to source's bounds -> Gives exact starting edge!
        int startX = std::clamp(dstCx, src.x, src.x + src.w);
        int startY = std::clamp(dstCy, src.y, src.y + src.h);

        // 3. Clamp source's center to target's bounds -> Gives exact ending edge!
        int endX = std::clamp(srcCx, dst.x, dst.x + dst.w);
        int endY = std::clamp(srcCy, dst.y, dst.y + dst.h);

        // Don't draw if they are somehow inside each other
        if (startX == endX && startY == endY) return;

        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);

        // 4. Draw the straight line
        SDL_RenderDrawLine(r, startX, startY, endX, endY);

        // 5. Draw the arrow head (always points perfectly down the line)
        float angle = atan2(endY - startY, endX - startX);
        int arrowLen = 14; // Fixed readable size

        SDL_RenderDrawLine(r, endX, endY, endX - arrowLen * cos(angle - M_PI / 6.0), endY - arrowLen * sin(angle - M_PI / 6.0));
        SDL_RenderDrawLine(r, endX, endY, endX - arrowLen * cos(angle + M_PI / 6.0), endY - arrowLen * sin(angle + M_PI / 6.0));

        // 6. Draw a tiny dot at the origin so you know which way it's flowing
        SDL_Rect dot = {startX - 2, startY - 2, 5, 5};
        SDL_RenderFillRect(r, &dot);
    }

    inline void DrawNavArrows(SDL_Renderer *r, SDL_Rect srcRect, const WidgetNav &nav, const std::unordered_map<std::string, SDL_Rect> &rects)
    {
        auto drawTo = [&](const std::string &targetId, SDL_Color color)
        {
            if (targetId.empty()) return;
            auto it = rects.find(targetId);
            if (it != rects.end())
            {
                DrawEdgeToEdgeArrow(r, srcRect, it->second, color);
            }
        };

        // Keep the original simple color scheme
        SDL_Color directionalColor = {255, 255, 0, 200}; // Yellow
        SDL_Color tabColor         = {0, 255, 255, 100}; // Cyan

        drawTo(nav.up,    directionalColor);
        drawTo(nav.down,  directionalColor);
        drawTo(nav.left,  directionalColor);
        drawTo(nav.right, directionalColor);
        drawTo(nav.next,  tabColor);
        drawTo(nav.prev,  tabColor);
    }
}
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

    // --- Placed at the end to not break positional {} initializations ---
    float expandX = 0.0f;
    float expandY = 0.0f;

    // Navigation & Keyboard
    WidgetNav nav;
    std::function<void()> onKeyPressFn;
    SDL_Keycode triggerKey = SDLK_RETURN; // Default Enter key
    bool enterTriggersClick = true;

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

    Widget OnClick(const std::string &animId, float durationSec, std::function<void()> cb, std::function<void(Widget &, float)> animFn, std::function<float(float)> curveFn = Easing::EaseOutQuad)
    {
        if (this->id.empty())
            this->id = animId;
        AnimState &anim = g_UIState[this->id];
        anim.clickDuration = durationSec;
        this->onClickFn = cb;
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
        WidgetDebug activeDebug = getActiveDebug(parentDebug);
        bool currentlyHovered = (input.mouseX >= rect.x && input.mouseX <= rect.x + rect.w && input.mouseY >= rect.y && input.mouseY <= rect.y + rect.h);

        bool isFocused = false;

        if (!this->id.empty())
        {
            g_WidgetRects[this->id] = rect; // Save position for drawing debug arrows

            isFocused = (g_FocusedWidgetId == this->id);
            if (g_DebugNavigation && isFocused)
            {
                // std::cout << "[FOCUS] " << this->id << " is active\n";
            }
            g_UIState[this->id].isFocused = isFocused;
            g_UIState[this->id].isHovered = currentlyHovered;

            if (currentlyHovered && input.mouseClicked)
            {
                g_UIState[this->id].isClicked = true;
                g_NextFocusedWidgetId = this->id; // Clicking automatically focuses this widget!
                if (onClickFn)
                    onClickFn();
            }

            // --- Keyboard Navigation Handling ---
            if (isFocused && input.keyPressed != SDLK_UNKNOWN)
            {
                if (input.keyPressed == SDLK_UP)
                {
                    NavLog(this->id, "UP", nav.up);
                    if (!nav.up.empty())
                        g_NextFocusedWidgetId = nav.up;
                }
                else if (input.keyPressed == SDLK_DOWN)
                {
                    NavLog(this->id, "DOWN", nav.down);
                    if (!nav.down.empty())
                        g_NextFocusedWidgetId = nav.down;
                }
                else if (input.keyPressed == SDLK_LEFT)
                {
                    NavLog(this->id, "LEFT", nav.left);
                    if (!nav.left.empty())
                        g_NextFocusedWidgetId = nav.left;
                }
                else if (input.keyPressed == SDLK_RIGHT)
                {
                    NavLog(this->id, "RIGHT", nav.right);
                    if (!nav.right.empty())
                        g_NextFocusedWidgetId = nav.right;
                }
                else if (input.keyPressed == SDLK_TAB)
                {
                    if (input.keyMod & KMOD_SHIFT)
                    {
                        NavLog(this->id, "SHIFT+TAB", nav.prev);
                        if (!nav.prev.empty())
                            g_NextFocusedWidgetId = nav.prev;
                    }
                    else
                    {
                        NavLog(this->id, "TAB", nav.next);
                        if (!nav.next.empty())
                            g_NextFocusedWidgetId = nav.next;
                    }
                }

                // Trigger Action
                if (input.keyPressed == this->triggerKey)
                {
                    if (this->enterTriggersClick && this->onClickFn)
                    {
                        std::cout << "[ACTION] " << this->id << " CLICK\n";
                        this->onClickFn();
                        g_UIState[this->id].isClicked = true;
                    }
                    else if (this->onKeyPressFn)
                    {
                        std::cout << "[ACTION] " << this->id << " KEY PRESS\n";
                        this->onKeyPressFn();
                    }
                }
            }
        }

        if (paint)
            paint(renderer, rect, style, input, children, activeDebug);

        // --- Debug Overlays ---
        if (activeDebug.enabled)
        {
            if (activeDebug.showBounds)
            {
                DebugDraw::DrawBoundsCross(renderer, rect, activeDebug.boundsColor);
            }
            if (activeDebug.showNavArrows && !this->id.empty())
            {
                DebugDraw::DrawNavArrows(renderer, rect, nav, g_WidgetRects);
            }
        }
    }
};