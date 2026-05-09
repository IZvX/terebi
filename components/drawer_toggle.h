#pragma once
#include <string>
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

    SDL_Color surface = GetThemeColor(DefaultTheme::SurfaceLight, wal.foreground, pywalEnabled);
    surface.a = 8;

    SDL_Color text = GetThemeColor(DefaultTheme::TextSecondary, wal.color7, pywalEnabled);

    SDL_Color hover = GetThemeColor(DefaultTheme::Hover, wal.color4, pywalEnabled);
    hover.a = 30;

    SDL_Color activeText = GetThemeColor(DefaultTheme::TextPrimary, wal.foreground, pywalEnabled);
    
    WidgetStyle style;
    style.color = surface;
    style.radius = 12;

    // --- Configure the Core Toggle Style using exactly the original app state ---
    ToggleStyle tStyle;
    tStyle.bgOff = GetThemeColor(DefaultTheme::SurfaceLight, wal.background, pywalEnabled);
    tStyle.bgOn  = GetThemeColor(DefaultTheme::AccentPrimary, wal.color1, pywalEnabled);
    
    // The original logic pulled SurfaceLight but forced the thumb to be fully opaque.
    // Without forcing alpha to 255 here, the knob becomes transparent and looks broken!
    tStyle.thumbOff = GetThemeColor(DefaultTheme::SurfaceLight, wal.foreground, pywalEnabled);
    tStyle.thumbOff.a = 255; 
    
    tStyle.thumbOn  = tStyle.thumbOff; // Same logic as the original code
    tStyle.thumbOn.a = 255;
    
    tStyle.thumbSize = 18;

    // Pass the rebuilt style into the core toggle
    Widget toggleSwitch = Toggle(id + "_switch", toggled, tStyle, nullptr, false);

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
                                // Exact original colors for icon and text
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

    w.OnClick(id + "_click", 0, [toggleAction]() { toggleAction(); },[](Widget&, float){});
    w.OnKeyPress(SDLK_RETURN, [toggleAction]() { toggleAction(); });

    // --- Focus & Hover Interaction (The White Ring) ---
    auto applyFocusStyle = [id](Widget &w, float t) {
        // High-contrast focus background (rgba 255,255,255, 0.1)
        w.animateColor({255, 255, 255, 25}, t);
        
        // The signature Nova OS white ring
        w.animateBorder({255, 255, 255, 255}, 3, t);
        
        // Traverse children to turn Icon and Text pure white
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
        .OnHover(id, 0.25f,[applyFocusStyle](Widget &w, float t) {
            SetCursor(CursorType::Hand);
            applyFocusStyle(w, t);
        })
        .OnFocus(id, 0.25f,[applyFocusStyle](Widget &w, float t) {
            applyFocusStyle(w, t);
        })
        .WithNav(nav.up, nav.down, nav.left, nav.right, nav.next, nav.prev)
    );
}