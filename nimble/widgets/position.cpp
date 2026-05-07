#pragma once
#include "../widget.h"

namespace Widgets
{
    inline Widget Position(PositionType type, Vector2 pos, Widget child)
    {
        // For Absolute and Fixed, size is 0x0 so it does NOT affect layout Flow (Row/Column)
        Widget w = {"", {0, 0}}; 
        
        // Use the native WidgetStyle position property as requested
        w.style.position = pos;

        // If Normal, it should take up space to push other widgets around in layout
        if (type == PositionType::Normal) {
            w.size = child.size;
            w.expandX = child.expandX;
            w.expandY = child.expandY;
        }

        w.children = {child};

        w.paint =[type](SDL_Renderer *renderer, SDL_Rect rect, const WidgetStyle &s, const InputState &input, const std::vector<Widget> &children, WidgetDebug debug)
        {
            if (children.empty()) return;
            const Widget &childWidget = children[0];

            if (type == PositionType::Fixed)
            {
                // --- FIXED ---
                // Store current clipping state
                SDL_Rect oldClip;
                bool clipEnabled = SDL_RenderIsClipEnabled(renderer);
                if (clipEnabled) {
                    SDL_RenderGetClipRect(renderer, &oldClip);
                }

                // Disable clipping so it can render anywhere on the screen
                SDL_RenderSetClipRect(renderer, nullptr);

                // Fixed uses pure screen coordinates from style.position
                SDL_Rect fixedRect = { 
                    (int)s.position.x, 
                    (int)s.position.y, 
                    (int)childWidget.size.x, 
                    (int)childWidget.size.y 
                };
                
                childWidget.render(renderer, fixedRect, input, debug);

                // Restore clipping state so other widgets aren't affected
                if (clipEnabled) {
                    SDL_RenderSetClipRect(renderer, &oldClip);
                }
            }
            else
            {
                // --- ABSOLUTE & NORMAL ---
                // `rect` already contains the parent's layout cursor + `s.position` 
                // because Widget::render internally computes (visualRect = rect + style.position).
                // Absolute achieves the floating effect simply because the wrapper has size {0,0}.
                SDL_Rect childRect = { 
                    rect.x, 
                    rect.y, 
                    (int)childWidget.size.x, 
                    (int)childWidget.size.y 
                };
                
                childWidget.render(renderer, childRect, input, debug);
            }
        };

        return w;
    }
}