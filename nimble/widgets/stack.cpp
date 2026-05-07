#pragma once
namespace Widgets {
    inline Widget Stack(const std::vector<Widget>& children, Alignment align = {0.5f, 0.5f}) {
        int maxW = 0, maxH = 0;
        for(const auto& c : children) { maxW = std::max(maxW, c.size.x); maxH = std::max(maxH, c.size.y); }
        return {"", {maxW, maxH}, {}, {}, children, [align](SDL_Renderer *renderer, SDL_Rect rect, const WidgetStyle&, const InputState& input, const std::vector<Widget>& childrenList, WidgetDebug dbg) {
            for(const auto& child : childrenList) {
                SDL_Rect cRect = {
                    rect.x + (int)((rect.w - child.size.x) * align.x), rect.y + (int)((rect.h - child.size.y) * align.y),
                    child.size.x, child.size.y
                };
                child.render(renderer, cRect, input, dbg);
            }
        }};
    }
}