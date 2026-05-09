#pragma once
#include <string>
#include <unordered_map>
#include <cmath>
#include <functional>
#include "../widget.h"

namespace Widgets
{

struct ToggleStyle {
    SDL_Color bgOff = {255, 255, 255, 25};     // Translucent background when Off
    SDL_Color bgOn = {255, 255, 255, 255};     // Solid background when On
    SDL_Color thumbOff = {161, 161, 170, 255}; // Gray knob when Off
    SDL_Color thumbOn = {0, 0, 0, 255};        // Black knob when On
    int thumbSize = 18;
};

inline Widget Toggle(
    const std::string& id, 
    bool& toggled, 
    const ToggleStyle& style = ToggleStyle{}, // App passes the style here
    std::function<void(bool)> onToggle = nullptr,
    bool interactive = true // Set to false if a parent container handles the click
) {
    Widget toggleSwitch = {"", {46, 24}};
    toggleSwitch.id = id;

    // Capture the style struct by value so it's baked into the paint lambda
    toggleSwitch.paint = [&toggled, id, style](SDL_Renderer* r, SDL_Rect rect, const WidgetStyle&, const InputState&, const std::vector<Widget>&, WidgetDebug) {
        static std::unordered_map<std::string, float> animStates;
        float& anim = animStates[id];

        float target = toggled ? 1.0f : 0.0f;
        anim += (target - anim) * 0.25f; // Slightly snappier animation
        
        if (std::abs(target - anim) < 0.005f) anim = target;
        
        SDL_Color currentBg = {
            (Uint8)(style.bgOff.r + (style.bgOn.r - style.bgOff.r) * anim),
            (Uint8)(style.bgOff.g + (style.bgOn.g - style.bgOff.g) * anim),
            (Uint8)(style.bgOff.b + (style.bgOn.b - style.bgOff.b) * anim),
            (Uint8)(style.bgOff.a + (style.bgOn.a - style.bgOff.a) * anim)
        };

        SDL_Color currentThumb = {
            (Uint8)(style.thumbOff.r + (style.thumbOn.r - style.thumbOff.r) * anim),
            (Uint8)(style.thumbOff.g + (style.thumbOn.g - style.thumbOff.g) * anim),
            (Uint8)(style.thumbOff.b + (style.thumbOn.b - style.thumbOff.b) * anim),
            (Uint8)(style.thumbOff.a + (style.thumbOn.a - style.thumbOff.a) * anim)
        };
        
        // Draw Track
        FillRoundedBoxAA(r, rect, rect.h / 2, currentBg);
        
        // Draw Knob
        int minX = rect.x + 3;
        int maxX = rect.x + rect.w - style.thumbSize - 3;
        int currentX = minX + (int)((maxX - minX) * anim);
        
        SDL_Rect thumbRect = {currentX, rect.y + 3, style.thumbSize, style.thumbSize};
        FillRoundedBoxAA(r, thumbRect, style.thumbSize / 2, currentThumb);
    };

    if (interactive) {
        auto toggleAction =[&toggled, onToggle]() { 
            toggled = !toggled;
            if(onToggle) onToggle(toggled); 
        };

        toggleSwitch.OnClick(id + "_click", 0, [toggleAction]() { toggleAction(); },[](Widget&, float){});
        toggleSwitch.OnKeyPress(SDLK_RETURN, [toggleAction]() { toggleAction(); });
        toggleSwitch.OnHover(id + "_hover", 0.1f,[](Widget& w, float t) {
            SetCursor(CursorType::Hand);
        });
    }

    return toggleSwitch;
}

} // namespace Widgets