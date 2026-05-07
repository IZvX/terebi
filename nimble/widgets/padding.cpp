#pragma once
#include <algorithm>
#include "expanded.cpp"
namespace Widgets {

    inline Widget Padding(Vector2 padding, Widget child,float expandX = 1,float expandY = 1)
    {
        return Expanded(expandY,expandX,
            {
                "",
                // Intrinsic size still exists, but is NOT used for layout decisions downstream
                { child.size.x + padding.x * 2,
                child.size.y + padding.y * 2 },

                {},
                {},
                { child },

                [padding](SDL_Renderer *r,
                        SDL_Rect rect,
                        const WidgetStyle&,
                        const InputState& in,
                        const std::vector<Widget>& childs,
                        WidgetDebug dbg)
                {
                    if (childs.empty())
                        return;

                    const auto& c = childs[0];

                    const int px = (int)padding.x;
                    const int py = (int)padding.y;

                    // Step 1: shrink parent space
                    SDL_Rect inner = {
                        rect.x + px,
                        rect.y + py,
                        std::max(0, rect.w - px * 2),
                        std::max(0, rect.h - py * 2)
                    };

                    // Step 2: resolve child size strictly within constraints
                    int cw = inner.w;
                    int ch = inner.h;

                    if (c.expandX > 0.0f)
                        cw = (int)(inner.w * c.expandX);
                    else
                        cw = std::min(c.size.x, inner.w);

                    if (c.expandY > 0.0f)
                        ch = (int)(inner.h * c.expandY);
                    else
                        ch = std::min(c.size.y, inner.h);

                    // Step 3: NO implicit centering
                    SDL_Rect cRect = {
                        inner.x,
                        inner.y,
                        cw,
                        ch
                    };

                    // Step 4: debug overlay
                    if (dbg.enabled && dbg.showPadding)
                    {
                        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
                        SDL_SetRenderDrawColor(
                            r,
                            dbg.paddingColor.r,
                            dbg.paddingColor.g,
                            dbg.paddingColor.b,
                            dbg.paddingColor.a
                        );

                        SDL_Rect top    = { rect.x, rect.y, rect.w, py };
                        SDL_Rect bottom = { rect.x, rect.y + rect.h - py, rect.w, py };
                        SDL_Rect left   = { rect.x, rect.y + py, px, rect.h - py * 2 };
                        SDL_Rect right  = { rect.x + rect.w - px, rect.y + py, px, rect.h - py * 2 };

                        SDL_RenderFillRect(r, &top);
                        SDL_RenderFillRect(r, &bottom);
                        SDL_RenderFillRect(r, &left);
                        SDL_RenderFillRect(r, &right);
                    }

                    // Step 5: render child inside constrained rect
                    c.render(r, cRect, in, dbg);
                },

                nullptr,
                child.expandX,
                child.expandY
            }
        );
    }

}