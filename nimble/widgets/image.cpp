#pragma once
enum ImageAlign {
    TopLeft, TopCenter, TopRight,
    MiddleLeft, MiddleCenter, MiddleRight,
    BottomLeft, BottomCenter, BottomRight
};

namespace Widgets {
    inline Widget Image(std::string path, Vector2 size, ObjectFit fit = ObjectFit::Cover, ImageAlign align = ImageAlign::MiddleCenter) {
        return {"", size, {}, {}, {}, [path, fit, align](SDL_Renderer* renderer, SDL_Rect rect, const WidgetStyle&, const InputState&, const std::vector<Widget>&, WidgetDebug dbg) {
            static std::unordered_map<std::string, SDL_Texture*> cache;
            if (cache.find(path) == cache.end()) {
                SDL_Surface* surf = IMG_Load(path.c_str());
                if (surf) {
                    cache[path] = SDL_CreateTextureFromSurface(renderer, surf);
                    SDL_SetTextureScaleMode(cache[path], SDL_SCALEMODE_LINEAR);
                    SDL_FreeSurface(surf);
                } else { cache[path] = nullptr; }
            }

            SDL_Texture* tex = cache[path];
            if (!tex) {
                SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                SDL_RenderFillRect(renderer, &rect);
                return;
            }

            int tw, th; SDL_QueryTexture(tex, NULL, NULL, &tw, &th);
            SDL_Rect src = {0, 0, tw, th};

            if (fit == ObjectFit::Cover) {
                float scale = std::max((float)rect.w / tw, (float)rect.h / th);
                src.w = (int)(rect.w / scale);
                src.h = (int)(rect.h / scale);

                // Horizontal alignment
                if      (align == TopLeft   || align == MiddleLeft   || align == BottomLeft)   src.x = 0;
                else if (align == TopRight  || align == MiddleRight  || align == BottomRight)  src.x = tw - src.w;
                else                                                                            src.x = (tw - src.w) / 2;

                // Vertical alignment
                if      (align == TopLeft   || align == TopCenter    || align == TopRight)     src.y = 0;
                else if (align == BottomLeft|| align == BottomCenter || align == BottomRight)  src.y = th - src.h;
                else                                                                            src.y = (th - src.h) / 2;

            } else if (fit == ObjectFit::Contain || fit == ObjectFit::ScaleDown) {
                float scale = (fit == ObjectFit::ScaleDown)
                    ? std::min(1.0f, std::min((float)rect.w / tw, (float)rect.h / th))
                    : std::min((float)rect.w / tw, (float)rect.h / th);

                int finalW = (int)(tw * scale);
                int finalH = (int)(th * scale);

                // Horizontal alignment
                if      (align == TopLeft   || align == MiddleLeft   || align == BottomLeft)   rect.x += 0;
                else if (align == TopRight  || align == MiddleRight  || align == BottomRight)  rect.x += rect.w - finalW;
                else                                                                            rect.x += (rect.w - finalW) / 2;

                // Vertical alignment
                if      (align == TopLeft   || align == TopCenter    || align == TopRight)     rect.y += 0;
                else if (align == BottomLeft|| align == BottomCenter || align == BottomRight)  rect.y += rect.h - finalH;
                else                                                                            rect.y += (rect.h - finalH) / 2;

                rect.w = finalW;
                rect.h = finalH;
            }
            SDL_RenderCopy(renderer, tex, &src, &rect);

            // Blend bottom portion with background color using a gradient
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            int blendHeight = rect.h / 4; // gradient fade zone at bottom
            int blendStartY = rect.y + rect.h - blendHeight;
            for (int i = 0; i < blendHeight; i++) {
                float t = (float)i / blendHeight;
                Uint8 alpha = (Uint8)(t * 255);
                SDL_SetRenderDrawColor(renderer, 50, 50, 50, alpha);
                SDL_RenderDrawLine(renderer, rect.x, blendStartY + i, rect.x + rect.w, blendStartY + i);
            }
        }};
    }
}