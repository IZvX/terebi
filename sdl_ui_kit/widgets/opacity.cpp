#pragma once
#include "../utils/helpers.h"

namespace Widgets {
    inline Widget OpacityWidget(Widget child, float opacity) {
        const float clampedOpacity = opacity < 0.0f ? 0.0f : (opacity > 1.0f ? 1.0f : opacity);

        Widget w = {
            "Opacity",
            child.size,
            {},
            {},
            {},
            [clampedOpacity](SDL_Renderer* renderer, SDL_Rect rect, const WidgetStyle& s, const InputState& input, const std::vector<Widget>& children, WidgetDebug dbg) {
                if (rect.w <= 0 || rect.h <= 0 || children.empty()) return;

                // Create a surface with the child rendered to it
                SDL_Surface* surface = SDL_CreateRGBSurface(0, rect.w, rect.h, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
                if (!surface) return;

                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                if (!texture) {
                    SDL_FreeSurface(surface);
                    return;
                }

                // Set alpha blending
                SDL_SetTextureAlphaMod(texture, (Uint8)(255.0f * clampedOpacity));
                
                SDL_RenderCopy(renderer, texture, nullptr, &rect);

                SDL_DestroyTexture(texture);
                SDL_FreeSurface(surface);
            }
        };

        if (child.paint) {
            w.children.push_back(std::move(child));
        }

        return w;
    }

    inline Widget Opacity(float opacity) {
        return [opacity](Widget child) -> Widget {
            return OpacityWidget(std::move(child), opacity);
        };
    }
}
