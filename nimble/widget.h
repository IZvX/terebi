

#pragma once
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <unordered_set>

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
    bool showExpanded = false;
    bool showRow = false;
    bool showColumn = false;
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
        if (startX == endX && startY == endY)
            return;

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
            if (targetId.empty())
                return;
            auto it = rects.find(targetId);
            if (it != rects.end())
            {
                DrawEdgeToEdgeArrow(r, srcRect, it->second, color);
            }
        };

        // Keep the original simple color scheme
        SDL_Color directionalColor = {255, 255, 0, 200}; // Yellow
        SDL_Color tabColor = {0, 255, 255, 100};         // Cyan

        drawTo(nav.up, directionalColor);
        drawTo(nav.down, directionalColor);
        drawTo(nav.left, directionalColor);
        drawTo(nav.right, directionalColor);
        drawTo(nav.next, tabColor);
        drawTo(nav.prev, tabColor);
    }
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
    bool lastHovered = false;
    bool lastFocused = false;
};
inline std::unordered_map<std::string, WidgetCache> g_WidgetCache;
inline std::unordered_map<std::string, WidgetNav> g_NavigationLinks;

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

            bool justGainedFocus = (!g_UIState[this->id].isFocused && (g_FocusedWidgetId == this->id));
            isFocused = (g_FocusedWidgetId == this->id);

            // Sync global state for animations (Progress is updated in UpdateUIAnimations)
            g_UIState[this->id].isFocused = isFocused;
            g_UIState[this->id].isHovered = currentlyHovered;

            if (justGainedFocus && boundText != nullptr)
                g_TextFieldState[this->id].selectionAnchor = g_TextFieldState[this->id].cursorPosition;

            // Handle Mouse Click & Focus Gain
            
                if (!this->disabled && currentlyHovered && input.mouseClicked)
                {
                    g_UIState[this->id].isClicked = true;
                    g_NextFocusedWidgetId = this->id;
                    if (onClickFn) onClickFn();                // --- 2.2 Input Capture Highlight ---
                if (g_Settings.highlightInputCapture)
                {
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 150); // Flash Green
                    SDL_RenderFillRect(renderer, &hitbox);
                }
            }

            // 3. KEYBOARD NAVIGATION HANDLING
            if (isFocused && !this->disabled && input.keyPressed != SDLK_UNKNOWN)
            {
                if (input.keyPressed == SDLK_UP) {
                    g_NextFocusedWidgetId = ResolveNavTarget(this->id, "up");
                    NavLog(this->id, "UP", g_NextFocusedWidgetId);
                }
                else if (input.keyPressed == SDLK_DOWN) {
                    g_NextFocusedWidgetId = ResolveNavTarget(this->id, "down");
                    NavLog(this->id, "DOWN", g_NextFocusedWidgetId);
                }
                else if (input.keyPressed == SDLK_LEFT) {
                    if (boundText == nullptr || (g_TextFieldState[this->id].cursorPosition == 0 && !(input.keyMod & KMOD_SHIFT))) {
                        g_NextFocusedWidgetId = ResolveNavTarget(this->id, "left");
                        NavLog(this->id, "LEFT", g_NextFocusedWidgetId);
                    }
                }
                else if (input.keyPressed == SDLK_RIGHT) {
                    if (boundText == nullptr || (g_TextFieldState[this->id].cursorPosition == (int)boundText->length() && !(input.keyMod & KMOD_SHIFT))) {
                        g_NextFocusedWidgetId = ResolveNavTarget(this->id, "right");
                        NavLog(this->id, "RIGHT", g_NextFocusedWidgetId);
                    }
                }
                else if (input.keyPressed == SDLK_TAB) {
                    if (input.keyMod & KMOD_SHIFT) {
                        g_NextFocusedWidgetId = ResolveNavTarget(this->id, "prev");
                        NavLog(this->id, "SHIFT+TAB", g_NextFocusedWidgetId);
                    } else {
                        g_NextFocusedWidgetId = ResolveNavTarget(this->id, "next");
                        NavLog(this->id, "TAB", g_NextFocusedWidgetId);
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
                if (input.enterPressed && onSubmitFn)
                    onSubmitFn(*boundText);
            }
        }

        // 5. PAINT
        // We pass the visualRect (the offset one) to the paint function
// Find "5. PAINT" in your Widget::render method and replace it with this:

        // 5. PAINT & CACHE
        if (this->useCache && !this->id.empty()) 
        {
            WidgetCache& cache = g_WidgetCache[this->id];
            bool dirty = false;

            // Determine if the widget's visuals need to be refreshed
            if (cache.lastW != visualRect.w || cache.lastH != visualRect.h) dirty = true;
            if (cache.lastHovered != currentlyHovered) dirty = true;
            if (cache.lastFocused != isFocused) dirty = true;
            // Any global keystroke or text typing usually means we should invalidate UI caches
            if (input.mouseClicked || input.keyPressed != SDLK_UNKNOWN || !input.textInput.empty()) dirty = true;
            if (g_UIState[this->id].isManuallyAnimated) dirty = true;

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

            if (activeDebug.showNavArrows && !this->id.empty())
            {
                DebugDraw::DrawNavArrows(renderer, visualRect, nav, g_WidgetRects);
            }
        }
    }
};

