#pragma once
namespace Widgets {

    // Overload 1: Solid color background (Your original code)
    inline Widget Scaffold(Widget body, SDL_Color bgColor = {255, 255, 255, 255}) {
        return {"", body.size, {}, {}, {body}, [bgColor](SDL_Renderer* renderer, SDL_Rect rect, const WidgetStyle&, const InputState& input, const std::vector<Widget>& children, WidgetDebug dbg) {
            SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, 255);
            SDL_RenderFillRect(renderer, &rect);
            if(!children.empty()) children[0].render(renderer, rect, input, dbg);
        }};
    }

    // Overload 2: Widget background (Image, DecoratedBox, etc.)
    inline Widget Scaffold(Widget body, Widget background) {
        // We store the background as children[0] and body as children[1]
        return {"", body.size, {}, {}, {background, body},[](SDL_Renderer* renderer, SDL_Rect rect, const WidgetStyle&, const InputState& input, const std::vector<Widget>& children, WidgetDebug dbg) {
            
            // 1. Render the background widget first so it is drawn behind the body.
            // Passing 'rect' makes it take up 100% of the Scaffold's size.
            if(children.size() > 0) {
                children[0].render(renderer, rect, input, dbg);
            }
            
            // 2. Render the body widget on top.
            if(children.size() > 1) {
                children[1].render(renderer, rect, input, dbg);
            }
        }};
    }

}