#pragma once
#include <cmath>

namespace Widgets {
    inline Widget Expanded(float vertical, float horizontal, Widget child) {
        child.expandY = std::max(0.0f, vertical);
        child.expandX = std::max(0.0f, horizontal);

        // 1. Store the child's original paint function
        auto originalPaint = child.paint;

        // 2. Wrap the paint function to add our Debug Overlay
        child.paint = [originalPaint, vertical, horizontal](
            SDL_Renderer* r, SDL_Rect rect, const WidgetStyle& style, 
            const InputState& input, const std::vector<Widget>& children, WidgetDebug debug) 
        {
            // Paint the original child first
            if (originalPaint) {
                originalPaint(r, rect, style, input, children, debug);
            }

            // Draw the Expanded Debug Overlay
            if (debug.enabled && debug.showExpanded) {
                SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
                SDL_Color expandColor = { 255, 165, 0, 200 }; // Orange
                SDL_SetRenderDrawColor(r, expandColor.r, expandColor.g, expandColor.b, expandColor.a);
                
                int cx = rect.x + rect.w / 2;
                int cy = rect.y + rect.h / 2;
                
                auto drawArrow = [&](int x1, int y1, int x2, int y2) {
                    SDL_RenderDrawLine(r, x1, y1, x2, y2);
                    float angle = atan2(y2 - y1, x2 - x1);
                    int arrowLen = 10;
                    SDL_RenderDrawLine(r, x2, y2, x2 - arrowLen * cos(angle - M_PI / 6.0), y2 - arrowLen * sin(angle - M_PI / 6.0));
                    SDL_RenderDrawLine(r, x2, y2, x2 - arrowLen * cos(angle + M_PI / 6.0), y2 - arrowLen * sin(angle + M_PI / 6.0));
                };

                // Horizontal expansion arrows
                if (horizontal > 0.0f && rect.w > 20) {
                    drawArrow(cx - 5, cy, rect.x, cy);
                    drawArrow(cx + 5, cy, rect.x + rect.w, cy);
                }
                
                // Vertical expansion arrows
                if (vertical > 0.0f && rect.h > 20) {
                    drawArrow(cx, cy - 5, cx, rect.y);
                    drawArrow(cx, cy + 5, cx, rect.y + rect.h);
                }

                SDL_RenderDrawRect(r, &rect);
            }
        };

        return child;
    }
}