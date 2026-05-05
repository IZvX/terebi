#pragma once
#include "../utils/helpers.h"

struct ToggleSliderStyle : WidgetStyle {
    float trackHeight = 8.0f;
    float thumbRadius = 12.0f;
    SDL_Color trackColorOff = {100, 100, 100, 255};
    SDL_Color trackColorOn = {76, 175, 80, 255};
    SDL_Color thumbColor = {255, 255, 255, 255};
};

namespace Widgets {
    inline Widget ToggleSlider(bool& value, const std::string& id, ToggleSliderStyle style = {}) {
        const float width = 60.0f;
        const float height = 32.0f;

        Widget w = {
            "ToggleSlider",
            {width, height},
            style,
            {},
            {},
            [style](SDL_Renderer* renderer, SDL_Rect rect, const WidgetStyle& s, const InputState& input, const std::vector<Widget>& children, WidgetDebug dbg) {
                if (rect.w <= 0 || rect.h <= 0) return;

                const ToggleSliderStyle& ts = static_cast<const ToggleSliderStyle&>(s);
                
                // Draw track
                SDL_Rect trackRect = {
                    rect.x + 5,
                    rect.y + (int)(rect.h / 2.0f - ts.trackHeight / 2.0f),
                    rect.w - 10,
                    (int)ts.trackHeight
                };
                
                SDL_Color trackColor = dbg.animatedState > 0.5f ? ts.trackColorOn : ts.trackColorOff;
                RoundFilledRect(renderer, trackRect, (int)(ts.trackHeight / 2.0f), trackColor);

                // Draw thumb
                float thumbX = rect.x + 8.0f + (dbg.animatedState * (rect.w - 16.0f - ts.thumbRadius * 2.0f));
                filledCircleRGBA(renderer, (int)(thumbX + ts.thumbRadius), rect.y + rect.h / 2, (int)ts.thumbRadius, 
                                ts.thumbColor.r, ts.thumbColor.g, ts.thumbColor.b, ts.thumbColor.a);
            }
        };

        w.id = id;
        return w
            .OnClick(id, 0.2f, []() {}, [&value](Widget& w, float t) {
                value = !value;
                w.animateState(value ? 1.0f : 0.0f, 0.15f);
            });
    }
}
