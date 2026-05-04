#pragma once
namespace Widgets {
    inline Widget Padding(Vector2 padding, Widget child) {
        return {"", {child.size.x + padding.x*2, child.size.y + padding.y*2}, {}, {}, {child},[padding](SDL_Renderer *r, SDL_Rect rect, const WidgetStyle&, const InputState& in, const std::vector<Widget>& childs, WidgetDebug dbg) {
            if(!childs.empty()) {
                const auto& c = childs[0];
                
                int pw = std::max(0, rect.w - padding.x*2);
                int ph = std::max(0, rect.h - padding.y*2);
                SDL_Rect pRect = {rect.x + padding.x, rect.y + padding.y, pw, ph};
                
                int cw = c.size.x;
                int ch = c.size.y;
                if (c.expandX > 0.0f) cw = (int)(pRect.w * c.expandX);
                else if (cw > pRect.w) cw = pRect.w;

                if (c.expandY > 0.0f) ch = (int)(pRect.h * c.expandY);
                else if (ch > pRect.h) ch = pRect.h;

                SDL_Rect cRect = {pRect.x + (pRect.w - cw)/2, pRect.y + (pRect.h - ch)/2, cw, ch};

                if (dbg.enabled && dbg.showPadding) {
                    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(r, dbg.paddingColor.r, dbg.paddingColor.g, dbg.paddingColor.b, dbg.paddingColor.a);
                    SDL_Rect t = {rect.x, rect.y, rect.w, padding.y};
                    SDL_Rect b = {rect.x, rect.y + rect.h - padding.y, rect.w, padding.y};
                    SDL_Rect l = {rect.x, rect.y + padding.y, padding.x, rect.h - padding.y*2};
                    SDL_Rect ri = {rect.x + rect.w - padding.x, rect.y + padding.y, padding.x, rect.h - padding.y*2};
                    SDL_RenderFillRect(r, &t); SDL_RenderFillRect(r, &b); 
                    SDL_RenderFillRect(r, &l); SDL_RenderFillRect(r, &ri);
                }
                
                c.render(r, cRect, in, dbg);
            }
        }};
    }
}