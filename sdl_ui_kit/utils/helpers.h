#pragma once

struct Alignment
{
    float x, y;
}; // 0.0 to 1.0

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


struct Gradient {
    bool enabled = false;
    bool isRadial = false;
    SDL_Color startColor = {255, 255, 255, 255};
    SDL_Color endColor = {0, 0, 0, 255};
    float angle = 0.0f;
};

struct ScrollbarStyle {
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
    Vector2 padding = {0,0};
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
    
    std::function<float(float)> curve = Easing::Linear;
};


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

// Declarations
void UpdateUIAnimations(float dt);
void ApplySurfaceGradient(SDL_Surface *surface, const Gradient &grad);
SDL_Texture *GetShadowTexture(SDL_Renderer *renderer, int w, int h, int radius, int spread, int blur);
void BoxBlurSurface(SDL_Surface *surf, int radius);
void FillRoundedBoxAA(SDL_Renderer *renderer, SDL_Rect rect, int radius, SDL_Color color, Gradient grad = {});