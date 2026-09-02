#pragma once
#include "math.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <cctype>
#include <utility>
#include "../sdl_compat.h"

struct Alignment
{
    float x, y;
}; // 0.0 to 1.0

enum class PositionType
{
    Absolute,
    Fixed,
    Normal
};

enum class MainAxisAlignment
{
    Start,
    Center,
    End,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly
};
enum class CrossAxisAlignment
{
    Start,
    Center,
    End,
    Stretch
};
enum class ObjectFit
{
    Cover,
    Contain,
    Fill,
    ScaleDown
};
enum class ScrollBehavior
{
    None,
    Auto,
    Always
};
enum class ScrollAxis
{
    None,
    Horizontal,
    Vertical,
    Both
};

struct Gradient
{
    bool enabled = false;
    bool isRadial = false;
    SDL_Color startColor = {255, 255, 255, 255};
    SDL_Color endColor = {0, 0, 0, 255};
    float angle = 0.0f;
};

struct ScrollbarStyle
{
    bool hidden = false;
    int thickness = 6;
    int radius = 3;
    SDL_Color trackColor = {0, 0, 0, 20};
    SDL_Color thumbColor = {100, 100, 100, 150};
    SDL_Color thumbHoverColor = {150, 150, 150, 200};
};
inline ScrollbarStyle g_GlobalScrollbarStyle = {};

struct WidgetStyle
{
    SDL_Color color = {0, 0, 0, 0};
    Gradient gradient;
    int radius = 0;

    int borderWidth = 0;
    Vector2 position = {0, 0};
    bool positionChangesBounds = false;
    Vector2 padding = {0, 0};
    SDL_Color borderColor = {0, 0, 0, 0};

    Vector2 shadowOffset = {0, 0};
    SDL_Color shadowColor = {0, 0, 0, 0};
    int shadowSpread = 0;
    int shadowBlur = 0;

    ScrollbarStyle scrollbarStyle = g_GlobalScrollbarStyle;
};

enum class NavAction
{
    Up,
    Down,
    Left,
    Right,
    Next,
    Prev,
    Back,
    Forward
};

struct NavigationEvent
{
    NavAction action = NavAction::Next;
    std::string label = "";
};

struct InputState
{
    bool anyEvent = false;
    int mouseX = 0;
    int mouseY = 0;
    Uint8 mouseButtonPressed = 0;
    bool mouseClicked = false;
    float mouseWheelX = 0.0f;
    float mouseWheelY = 0.0f;
    bool globalDebugMode = true;
    SDL_Keycode keyPressed = SDLK_UNKNOWN;
    Uint16 keyMod = 0;

    // --- New Text Input Fields ---
    std::string textInput = "";
    bool backspacePressed = false;
    bool deletePressed = false;
    bool leftPressed = false;
    bool rightPressed = false;
    bool enterPressed = false;
    bool leftMouseDown = false;
    bool rightMouseDown = false;
    bool rightMouseClicked = false;
    std::vector<NavigationEvent> navigationEvents;
};

struct NavigationBinding
{
    enum class Source
    {
        Key,
        MouseButton,
        Flag,
        Predicate,
        Callback
    };

    Source source = Source::Key;
    NavAction action = NavAction::Next;
    SDL_Keycode key = SDLK_UNKNOWN;
    Uint16 requiredMod = 0;
    Uint16 rejectedMod = 0;
    Uint8 mouseButton = 0;
    bool *flag = nullptr;
    bool consumeFlag = false;
    std::function<bool(const InputState &)> predicate;
    std::function<bool()> callback;
    std::string label = "";
};

inline std::vector<NavigationBinding> g_NavigationBindings;
inline std::vector<std::pair<NavAction, std::function<void()>>> g_NavigationActionCallbacks;
inline std::vector<NavigationEvent> g_QueuedNavigationActions;

inline const char *NavActionName(NavAction action)
{
    switch (action)
    {
    case NavAction::Up: return "UP";
    case NavAction::Down: return "DOWN";
    case NavAction::Left: return "LEFT";
    case NavAction::Right: return "RIGHT";
    case NavAction::Next: return "NEXT";
    case NavAction::Prev: return "PREV";
    case NavAction::Back: return "BACK";
    case NavAction::Forward: return "FORWARD";
    }
    return "NAV";
}

inline bool NavigationModsMatch(Uint16 current, Uint16 required, Uint16 rejected)
{
    return (current & required) == required && (current & rejected) == 0;
}

inline void QueueNavigationAction(InputState &input, NavAction action, const std::string &label = "")
{
    input.navigationEvents.push_back({action, label.empty() ? NavActionName(action) : label});

    for (auto &entry : g_NavigationActionCallbacks)
    {
        if (entry.first == action && entry.second)
            entry.second();
    }
}

inline void ClearNavigationMappings()
{
    g_NavigationBindings.clear();
}

inline void ClearNavigationActionCallbacks()
{
    g_NavigationActionCallbacks.clear();
}

inline void MapNavigationKey(NavAction action, SDL_Keycode key, Uint16 requiredMod = 0, Uint16 rejectedMod = 0, const std::string &label = "")
{
    g_NavigationBindings.push_back({
        NavigationBinding::Source::Key,
        action,
        key,
        requiredMod,
        rejectedMod,
        0,
        nullptr,
        false,
        {},
        {},
        label.empty() ? SDL_GetKeyName(key) : label
    });
}

inline void MapNavigationMouseButton(NavAction action, Uint8 mouseButton, const std::string &label = "")
{
    g_NavigationBindings.push_back({
        NavigationBinding::Source::MouseButton,
        action,
        SDLK_UNKNOWN,
        0,
        0,
        mouseButton,
        nullptr,
        false,
        {},
        {},
        label.empty() ? "MOUSE" : label
    });
}

inline void MapNavigationFlag(NavAction action, bool *flag, bool consume = true, const std::string &label = "")
{
    g_NavigationBindings.push_back({
        NavigationBinding::Source::Flag,
        action,
        SDLK_UNKNOWN,
        0,
        0,
        0,
        flag,
        consume,
        {},
        {},
        label.empty() ? "FLAG" : label
    });
}

inline void MapNavigationPredicate(NavAction action, std::function<bool(const InputState &)> predicate, const std::string &label = "")
{
    g_NavigationBindings.push_back({
        NavigationBinding::Source::Predicate,
        action,
        SDLK_UNKNOWN,
        0,
        0,
        0,
        nullptr,
        false,
        std::move(predicate),
        {},
        label.empty() ? "PREDICATE" : label
    });
}

inline void MapNavigationCallback(NavAction action, std::function<bool()> callback, const std::string &label = "")
{
    g_NavigationBindings.push_back({
        NavigationBinding::Source::Callback,
        action,
        SDLK_UNKNOWN,
        0,
        0,
        0,
        nullptr,
        false,
        {},
        std::move(callback),
        label.empty() ? "CALLBACK" : label
    });
}

inline void OnNavigationAction(NavAction action, std::function<void()> callback)
{
    g_NavigationActionCallbacks.push_back({action, std::move(callback)});
}

inline void TriggerNavigationAction(NavAction action, const std::string &label = "")
{
    g_QueuedNavigationActions.push_back({action, label.empty() ? NavActionName(action) : label});
}

inline void ResetDefaultNavigationMappings()
{
    ClearNavigationMappings();
    MapNavigationKey(NavAction::Up, SDLK_UP, 0, 0, "UP");
    MapNavigationKey(NavAction::Down, SDLK_DOWN, 0, 0, "DOWN");
    MapNavigationKey(NavAction::Left, SDLK_LEFT, 0, 0, "LEFT");
    MapNavigationKey(NavAction::Right, SDLK_RIGHT, 0, 0, "RIGHT");
    MapNavigationKey(NavAction::Prev, SDLK_TAB, SDL_KMOD_SHIFT, 0, "SHIFT+TAB");
    MapNavigationKey(NavAction::Next, SDLK_TAB, 0, SDL_KMOD_SHIFT, "TAB");
    MapNavigationKey(NavAction::Back, SDLK_ESCAPE, 0, 0, "ESC");
    MapNavigationKey(NavAction::Forward, SDLK_F1, 0, 0, "F1");
    MapNavigationMouseButton(NavAction::Back, SDL_BUTTON_X1, "MOUSE_BACK");
    MapNavigationMouseButton(NavAction::Forward, SDL_BUTTON_X2, "MOUSE_FORWARD");
}

inline void EvaluateNavigationMappings(InputState &input)
{
    for (const NavigationEvent &event : g_QueuedNavigationActions)
        QueueNavigationAction(input, event.action, event.label);
    g_QueuedNavigationActions.clear();

    for (auto &binding : g_NavigationBindings)
    {
        bool triggered = false;

        switch (binding.source)
        {
        case NavigationBinding::Source::Key:
            triggered = input.keyPressed == binding.key &&
                        NavigationModsMatch(input.keyMod, binding.requiredMod, binding.rejectedMod);
            break;
        case NavigationBinding::Source::MouseButton:
            triggered = input.mouseButtonPressed == binding.mouseButton;
            break;
        case NavigationBinding::Source::Flag:
            triggered = binding.flag != nullptr && *binding.flag;
            if (triggered && binding.consumeFlag)
                *binding.flag = false;
            break;
        case NavigationBinding::Source::Predicate:
            triggered = binding.predicate && binding.predicate(input);
            break;
        case NavigationBinding::Source::Callback:
            triggered = binding.callback && binding.callback();
            break;
        }

        if (triggered)
            QueueNavigationAction(input, binding.action, binding.label);
    }
}

struct AnimState
{
    bool isHovered = false;
    float hoverProgress = 0.0f;
    float hoverDuration = 0.2f;
    bool isClicked = false;
    float clickProgress = 0.0f;
    float clickDuration = 0.2f;
    bool isFocused = false;
    bool isSecondaryFocused = false;
    float focusProgress = 0.0f;
    float focusDuration = 0.2f;

    // --- Manual Animation State ---
    bool isManuallyAnimated = false;
    float manualProgress = 0.0f;
    float manualDuration = 0.2f;
    std::function<float(float)> curve = Easing::Linear;

    // --- Dirty Repaint ---
    bool isDirty = true;          // starts true so first frame always paints
    size_t lastTextHash = 0;      // detects external boundText mutations
};

// --- New Global TextField State Map ---
struct TextFieldState
{
    int cursorPosition = 0;
    int selectionAnchor = 0;
    bool isDragging = false;
    bool contextMenuOpen = false;
    SDL_Point contextMenuPos = {0, 0};
    SDL_Rect lastRect = {0, 0, 0, 0}; // Stored during paint pass for mouse detection

    bool HasSelection() const { return cursorPosition != selectionAnchor; }
    int GetSelectionStart() const { return std::min(cursorPosition, selectionAnchor); }
    int GetSelectionEnd() const { return std::max(cursorPosition, selectionAnchor); }

    void ClampToText(const std::string &text)
    {
        int len = (int)text.length();
        cursorPosition = std::clamp(cursorPosition, 0, len);
        selectionAnchor = std::clamp(selectionAnchor, 0, len);
    }

    void DeleteSelection(std::string &text)
    {
        ClampToText(text);
        if (!HasSelection())
            return;
        int start = GetSelectionStart();
        int end = GetSelectionEnd();
        text.erase(start, end - start);
        cursorPosition = selectionAnchor = start;
    }
};

inline std::string GetSelectionText(const std::string &text, const TextFieldState &tfState)
{
    if (!tfState.HasSelection())
        return "";
    return text.substr(tfState.GetSelectionStart(), tfState.GetSelectionEnd() - tfState.GetSelectionStart());
}

struct TextFieldStyle
{
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Color placeholderColor = {150, 150, 150, 255};
    SDL_Color backgroundColor = {30, 30, 30, 0};
    SDL_Color cursorColor = {255, 255, 255, 255};
    SDL_Color borderColor = {100, 100, 100, 255};
    SDL_Color focusedBorderColor = {0, 150, 255, 0};
    int borderWidth = 1;
    int radius = 4;
    Vector2 padding = {10, 5};
    TTF_Font *font = nullptr;
};

// Declare globals before they're referenced by inline helpers.
extern std::unordered_map<std::string, AnimState> g_UIState;
extern std::unordered_map<std::string, TextFieldState> g_TextFieldState;
extern std::unordered_map<std::string, std::string *> g_TextFieldBindings;

// --- UTF-8 Aware Movement ---
inline void MoveCursorLeft(const std::string &text, int &pos)
{
    if (pos <= 0)
    {
        pos = 0;
        return;
    }
    pos--;
    while (pos > 0 && (text[pos] & 0xC0) == 0x80)
        pos--;
}

inline void MoveCursorRight(const std::string &text, int &pos)
{
    int len = text.length();
    if (pos >= len)
    {
        pos = len;
        return;
    }
    pos++;
    while (pos < len && (text[pos] & 0xC0) == 0x80)
        pos++;
}

// --- Word Boundary Detection ---
inline int FindWordBoundaryLeft(const std::string &text, int pos)
{
    if (pos <= 0)
        return 0;
    pos--;
    while (pos > 0 && std::isspace(static_cast<unsigned char>(text[pos])))
        pos--;
    while (pos > 0 && !std::isspace(static_cast<unsigned char>(text[pos - 1])))
        pos--;
    return pos;
}

inline int FindWordBoundaryRight(const std::string &text, int pos)
{
    int len = text.length();
    if (pos >= len)
        return len;
    while (pos < len && !std::isspace(static_cast<unsigned char>(text[pos])))
        pos++;
    while (pos < len && std::isspace(static_cast<unsigned char>(text[pos])))
        pos++;
    return pos;
}

// --- Mouse X-to-Text-Index ---
inline int GetTextIndexFromMouse(TTF_Font *font, const std::string &text, int relX)
{
    if (text.empty() || relX <= 0)
        return 0;
    int w = 0, h = 0, lastW = 0, lastValid = 0;
    for (size_t i = 1; i <= text.length(); ++i)
    {
        if (i < text.length() && (text[i] & 0xC0) == 0x80)
            continue; // Skip continuation bytes
        TTF_SizeUTF8(font, text.substr(0, i).c_str(), &w, &h);
        if (w >= relX)
            return (relX - lastW < w - relX) ? lastValid : (int)i; // Snap to closest
        lastW = w;
        lastValid = i;
    }
    return (int)text.length();
}

// Call this inside your SDL event polling loop. Returns true if the key/mouse was consumed.
inline bool ProcessTextFieldEvent(const std::string &id, std::string &text, const SDL_Event &e, const TextFieldStyle &tfStyle)
{
    if (!g_UIState[id].isFocused)
        return false;

    TextFieldState &tfState = g_TextFieldState[id];
    SDL_Rect rect = tfState.lastRect;

    // Safety bounds
    if (tfState.cursorPosition > text.length())
        tfState.cursorPosition = text.length();
    if (tfState.selectionAnchor > text.length())
        tfState.selectionAnchor = text.length();

    bool shift = SDL_GetModState() & SDL_KMOD_SHIFT;
    bool ctrl = SDL_GetModState() & SDL_KMOD_CTRL;

    // --- MOUSE SELECTING ---
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        if (e.button.x >= rect.x && e.button.x <= rect.x + rect.w && e.button.y >= rect.y && e.button.y <= rect.y + rect.h)
        {
            int relX = e.button.x - (rect.x + tfStyle.padding.x);
            tfState.cursorPosition = tfState.selectionAnchor = GetTextIndexFromMouse(tfStyle.font, text, relX);
            tfState.isDragging = true;
            return true;
        }
    }
    else if (e.type == SDL_EVENT_MOUSE_MOTION && tfState.isDragging)
    {
        int relX = e.motion.x - (rect.x + tfStyle.padding.x);
        tfState.cursorPosition = GetTextIndexFromMouse(tfStyle.font, text, relX);
        return true;
    }
    else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT)
    {
        tfState.isDragging = false;
    }

    // --- KEYBOARD ACTIONS ---
    else if (e.type == SDL_EVENT_KEY_DOWN)
    {
        switch (e.key.key)
        {
        case SDLK_LEFT:
        {
            // IF AT BOUNDARY -> ALLOW FOCUS MANAGER TO TAKE OVER
            if (tfState.cursorPosition == 0 && !tfState.HasSelection())
                return false;

            if (!shift && tfState.HasSelection())
            {
                tfState.cursorPosition = tfState.selectionAnchor = tfState.GetSelectionStart();
            }
            else
            {
                if (ctrl)
                    tfState.cursorPosition = FindWordBoundaryLeft(text, tfState.cursorPosition);
                else
                    MoveCursorLeft(text, tfState.cursorPosition);
            }
            if (!shift)
                tfState.selectionAnchor = tfState.cursorPosition;
            return true;
        }
        case SDLK_RIGHT:
        {
            // IF AT BOUNDARY -> ALLOW FOCUS MANAGER TO TAKE OVER
            if (tfState.cursorPosition == text.length() && !tfState.HasSelection())
                return false;

            if (!shift && tfState.HasSelection())
            {
                tfState.cursorPosition = tfState.selectionAnchor = tfState.GetSelectionEnd();
            }
            else
            {
                if (ctrl)
                    tfState.cursorPosition = FindWordBoundaryRight(text, tfState.cursorPosition);
                else
                    MoveCursorRight(text, tfState.cursorPosition);
            }
            if (!shift)
                tfState.selectionAnchor = tfState.cursorPosition;
            return true;
        }
        case SDLK_BACKSPACE:
        {
            if (tfState.HasSelection())
            {
                tfState.DeleteSelection(text);
            }
            else
            {
                if (ctrl)
                { // Delete word behind
                    int bound = FindWordBoundaryLeft(text, tfState.cursorPosition);
                    text.erase(bound, tfState.cursorPosition - bound);
                    tfState.cursorPosition = tfState.selectionAnchor = bound;
                }
                else if (tfState.cursorPosition > 0)
                { // Standard backspace
                    int prev = tfState.cursorPosition;
                    MoveCursorLeft(text, tfState.cursorPosition);
                    text.erase(tfState.cursorPosition, prev - tfState.cursorPosition);
                    tfState.selectionAnchor = tfState.cursorPosition;
                }
            }
            return true;
        }
        }
    }
    // --- TYPING NEW TEXT ---
    else if (e.type == SDL_EVENT_TEXT_INPUT)
    {
        if (tfState.HasSelection())
            tfState.DeleteSelection(text);
        text.insert(tfState.cursorPosition, e.text.text);
        tfState.cursorPosition += strlen(e.text.text);
        tfState.selectionAnchor = tfState.cursorPosition;
        return true;
    }
    return false;
}


struct ScrollState
{
    float scrollX = 0, scrollY = 0;
    float targetX = 0, targetY = 0;
};

struct IconData
{
    uint32_t codepoint;
};

inline std::unordered_map<std::string, AnimState> g_UIState;
inline std::unordered_map<std::string, ScrollState> g_ScrollState;
inline std::unordered_map<std::string, TextFieldState> g_TextFieldState;
inline std::unordered_map<std::string, std::string *> g_TextFieldBindings;
inline std::unordered_map<std::string, SDL_Texture *> g_ClipTextureCache;

inline bool InsertTextIntoTextField(const std::string &id, const std::string &value)
{
    auto binding = g_TextFieldBindings.find(id);
    if (binding == g_TextFieldBindings.end() || !binding->second || value.empty())
        return false;

    std::string &text = *binding->second;
    TextFieldState &tfState = g_TextFieldState[id];
    tfState.ClampToText(text);

    if (tfState.HasSelection())
        tfState.DeleteSelection(text);

    tfState.ClampToText(text);
    text.insert((size_t)tfState.cursorPosition, value);
    tfState.cursorPosition += (int)value.length();
    tfState.selectionAnchor = tfState.cursorPosition;
    tfState.ClampToText(text);
    return true;
}

inline bool BackspaceTextField(const std::string &id)
{
    auto binding = g_TextFieldBindings.find(id);
    if (binding == g_TextFieldBindings.end() || !binding->second)
        return false;

    std::string &text = *binding->second;
    TextFieldState &tfState = g_TextFieldState[id];
    tfState.ClampToText(text);

    if (tfState.HasSelection())
    {
        tfState.DeleteSelection(text);
        return true;
    }

    if (tfState.cursorPosition <= 0)
        return true;

    int previous = tfState.cursorPosition;
    MoveCursorLeft(text, tfState.cursorPosition);
    text.erase((size_t)tfState.cursorPosition, (size_t)(previous - tfState.cursorPosition));
    tfState.selectionAnchor = tfState.cursorPosition;
    return true;
}


inline SDL_Texture *GetClipTexture(SDL_Renderer *r, int w, int h, const std::string &id)
{
    if (g_ClipTextureCache.count(id))
    {
        int tw, th;
        SDL_QueryTexture(g_ClipTextureCache[id], NULL, NULL, &tw, &th);
        if (tw == w && th == h)
            return g_ClipTextureCache[id];
        SDL_DestroyTexture(g_ClipTextureCache[id]);
    }
    g_ClipTextureCache[id] = SDL_CreateTexture(r, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, w, h);
    SDL_SetTextureBlendMode(g_ClipTextureCache[id], SDL_BLENDMODE_BLEND);
    return g_ClipTextureCache[id];
}
// Declarations
void UpdateUIAnimations(float dt);
void ApplySurfaceGradient(SDL_Surface *surface, const Gradient &grad);
SDL_Texture *GetShadowTexture(SDL_Renderer *renderer, int w, int h, int radius, int spread, int blur);
void BoxBlurSurface(SDL_Surface *surf, int radius);
void FillRoundedBoxAA(SDL_Renderer *renderer, SDL_Rect rect, int radius, SDL_Color color, Gradient grad = {});
