#pragma once
namespace Widgets {
    inline Widget Center(Widget child) {
        return {"", child.size, {}, {}, {child},[](SDL_Renderer *renderer, SDL_Rect rect, const WidgetStyle& s, const InputState& input, const std::vector<Widget>& childrenList, WidgetDebug dbg) {
            if (!childrenList.empty()) {
                const auto& c = childrenList[0];
                int cw = c.size.x;
                int ch = c.size.y;
                
                if (c.expandX > 0.0f) cw = (int)(rect.w * c.expandX);
                else if (cw > rect.w) cw = rect.w; 

                if (c.expandY > 0.0f) ch = (int)(rect.h * c.expandY);
                else if (ch > rect.h) ch = rect.h; 

                SDL_Rect cRect = {rect.x + (rect.w - cw)/2, rect.y + (rect.h - ch)/2, cw, ch};
                c.render(renderer, cRect, input, dbg);
            }
        }};
    }
}