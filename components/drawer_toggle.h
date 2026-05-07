#pragma once
#include <string>
#include <unordered_map>
#include <cmath>
#include <functional>
#include "shared.h"

inline Widget DrawerToggle(
    bool& toggled, 
    uint32_t iconCode, 
    const std::string &label, 
    const std::string &id, 
    const WidgetNav &nav, 
    AppFonts& fonts, 
    std::function<void(bool)> onToggle = nullptr
) {
    using namespace Widgets;
        bool pywalEnabled = g_Context.pywalEnabled;
    WalTheme wal = g_Context.currentTheme;

        SDL_Color surface = GetThemeColor(
        DefaultTheme::SurfaceLight,
        wal.foreground,
        pywalEnabled
    );

    surface.a = 8;

    SDL_Color text = GetThemeColor(
        DefaultTheme::TextSecondary,
        wal.color7,
        pywalEnabled
    );

    SDL_Color hover = GetThemeColor(
        DefaultTheme::Hover,
        wal.color4,
        pywalEnabled
    );

    hover.a = 30;

    SDL_Color activeText = GetThemeColor(
        DefaultTheme::TextPrimary,
        wal.foreground,
        pywalEnabled
    );

    
    WidgetStyle style;
    style.color = surface;
    style.radius = 12;

    // --- The Toggle Switch Widget ---
    Widget toggleSwitch = {"", {46, 24}};
    toggleSwitch.paint = [&toggled, id,pywalEnabled,wal](SDL_Renderer* r, SDL_Rect rect, const WidgetStyle&, const InputState&, const std::vector<Widget>&, WidgetDebug) {
        static std::unordered_map<std::string, float> animStates;
        float& anim = animStates[id];

        float target = toggled ? 1.0f : 0.0f;
        anim += (target - anim) * 0.25f; // Slightly snappier animation
        
        if (std::abs(target - anim) < 0.005f) anim = target;

        // Monochrome Logic: 
        // Track: Translucent White (Off) -> Solid White (On)
        SDL_Color bgOff = GetThemeColor(
            DefaultTheme::SurfaceLight,
            wal.background,
            pywalEnabled
        );

        // SDL_Color bgOff = {255, 255, 255, 25};  // rgba(255,255,255,0.1)
        SDL_Color bgOn  = GetThemeColor(
            DefaultTheme::AccentPrimary,
            wal.color1,
            pywalEnabled
        ); // #ffffff
        
        // Knob: Gray (Off) -> Black (On)
        SDL_Color thumbOff = GetThemeColor(
            DefaultTheme::SurfaceLight,
            wal.foreground,
            pywalEnabled
        ); // #a1a1aa
        SDL_Color thumbOn  = thumbOff;       // #000000
        
        SDL_Color currentBg = {
            (Uint8)(bgOff.r + (bgOn.r - bgOff.r) * anim),
            (Uint8)(bgOff.g + (bgOn.g - bgOff.g) * anim),
            (Uint8)(bgOff.b + (bgOn.b - bgOff.b) * anim),
            (Uint8)(bgOff.a + (bgOn.a - bgOff.a) * anim)
        };

        SDL_Color currentThumb = {
            (Uint8)(thumbOff.r + (thumbOn.r - thumbOff.r) * anim),
            (Uint8)(thumbOff.g + (thumbOn.g - thumbOff.g) * anim),
            (Uint8)(thumbOff.b + (thumbOn.b - thumbOff.b) * anim),
            255
        };
        
        // Draw Track
        FillRoundedBoxAA(r, rect, rect.h / 2, currentBg);
        
        // Draw Knob
        int thumbSize = 18;
        int minX = rect.x + 3;
        int maxX = rect.x + rect.w - thumbSize - 3;
        int currentX = minX + (int)((maxX - minX) * anim);
        
        SDL_Rect thumbRect = {currentX, rect.y + 3, thumbSize, thumbSize};
        FillRoundedBoxAA(r, thumbRect, thumbSize / 2, currentThumb);
    };

    // --- Layout Construction ---
    Widget w =
        RoundedBox(
            {},
            style,
            Padding({20, 18},
                Row(
                    MainAxisAlignment::SpaceBetween,
                    CrossAxisAlignment::Center,
                    0,
                    {
                        Row(
                            MainAxisAlignment::Start,
                            CrossAxisAlignment::Center,
                            20, 
                            {
                                // Gray text/icon by default
                                Icon({iconCode}, fonts.fontAwesome24, 24, {161, 161, 170, 255}),
                                Text(label, fonts.arial18, {161, 161, 170, 255})
                            }
                        ),
                        SizedBox({}),
                        toggleSwitch 
                    },ScrollBehavior::None,ScrollAxis::Horizontal,"",1,1
                )
            ),
            {-1, 0}
        );

    w.id = id;
    
    auto toggleAction = [&toggled, onToggle]() { 
        toggled = !toggled;
        if(onToggle) onToggle(toggled); 
    };

    w.OnClick(id + "_click", 0, [toggleAction]() { toggleAction(); }, [](Widget&, float){});
    w.OnKeyPress(SDLK_RETURN, [toggleAction]() { toggleAction(); });

    // --- Focus & Hover Interaction (The White Ring) ---
    auto applyFocusStyle = [id](Widget &w, float t) {
        // High-contrast focus background (rgba 255,255,255, 0.1)
        w.animateColor({255, 255, 255, 25}, t);
        
        // The signature Nova OS white ring
        w.animateBorder({255, 255, 255, 255}, 3, t);
        
        // Traverse children to turn Icon and Text pure white
        // Structure: RoundedBox -> Expanded -> Padding -> Row -> Row(Icons)
        try {
            auto& contentRow = w.children[0].children[0].children[0].children[0];
            if (contentRow.children.size() > 0) {
                auto& labelGroup = contentRow.children[0];
                labelGroup.children[0].animateColor({255, 255, 255, 255}, t); // Icon
                labelGroup.children[1].animateColor({255, 255, 255, 255}, t); // Text
            }
        } catch (...) {}
    };

    return Expanded(0, 1, w
        .OnHover(id, 0.25f, [applyFocusStyle](Widget &w, float t) {
            SetCursor(CursorType::Hand);
            applyFocusStyle(w, t);
        })
        .OnFocus(id, 0.25f, [applyFocusStyle](Widget &w, float t) {
            applyFocusStyle(w, t);
        })
        .WithNav(nav.up, nav.down, nav.left, nav.right, nav.next, nav.prev)
    );
}