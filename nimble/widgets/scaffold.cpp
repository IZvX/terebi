#pragma once
namespace Widgets {

    // Overload 1: Color background (Defaults to transparent {0,0,0,0})
    inline Widget Scaffold(Widget body, SDL_Color bgColor = {0, 0, 0, 0}) {
        return {"", body.size, {}, {}, {body}, [bgColor](SDL_Renderer* renderer, SDL_Rect rect, const WidgetStyle&, const InputState& input, const std::vector<Widget>& children, WidgetDebug dbg) {
            // Only draw a fill if alpha > 0 (transparent by default)
            if (bgColor.a > 0) {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
                SDL_RenderFillRect(renderer, &rect);
            }
            if(!children.empty()) children[0].render(renderer, rect, input, dbg);
        }};
    }

    // Overload 2: Widget background (Image, DecoratedBox, tint, etc.)
    inline Widget Scaffold(Widget body, Widget background) {
        return {"", body.size, {}, {}, {background, body}, [](SDL_Renderer* renderer, SDL_Rect rect, const WidgetStyle&, const InputState& input, const std::vector<Widget>& children, WidgetDebug dbg) {
            // 1. Render background widget first
            if(children.size() > 0) {
                children[0].render(renderer, rect, input, dbg);
            }
            
            // 2. Render body on top
            if(children.size() > 1) {
                children[1].render(renderer, rect, input, dbg);
            }
        }};
    }

}