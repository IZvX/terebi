#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <cctype>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

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

struct InputState
{
    int mouseX = 0;
    int mouseY = 0;
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
};

struct AnimState
{
    bool isHovered = false;
    float hoverProgress = 0.0f;
    float hoverDuration = 0.2f;
    bool isClicked = false;
    float clickProgress = 0.0f;
    float clickDuration = 0.2f;
    bool isFocused = false;
    float focusProgress = 0.0f;
    float focusDuration = 0.2f;

    // --- Manual Animation State ---
    bool isManuallyAnimated = false; // toggle state
    float manualProgress = 0.0f;
    float manualDuration = 0.2f;

    std::function<float(float)> curve = Easing::Linear;
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

    void DeleteSelection(std::string &text)
    {
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

    bool shift = SDL_GetModState() & KMOD_SHIFT;
    bool ctrl = SDL_GetModState() & KMOD_CTRL;

    // --- MOUSE SELECTING ---
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        if (e.button.x >= rect.x && e.button.x <= rect.x + rect.w && e.button.y >= rect.y && e.button.y <= rect.y + rect.h)
        {
            int relX = e.button.x - (rect.x + tfStyle.padding.x);
            tfState.cursorPosition = tfState.selectionAnchor = GetTextIndexFromMouse(tfStyle.font, text, relX);
            tfState.isDragging = true;
            return true;
        }
    }
    else if (e.type == SDL_MOUSEMOTION && tfState.isDragging)
    {
        int relX = e.motion.x - (rect.x + tfStyle.padding.x);
        tfState.cursorPosition = GetTextIndexFromMouse(tfStyle.font, text, relX);
        return true;
    }
    else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
    {
        tfState.isDragging = false;
    }

    // --- KEYBOARD ACTIONS ---
    else if (e.type == SDL_KEYDOWN)
    {
        switch (e.key.keysym.sym)
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
    else if (e.type == SDL_TEXTINPUT)
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
inline std::unordered_map<std::string, SDL_Texture *> g_ClipTextureCache;

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
