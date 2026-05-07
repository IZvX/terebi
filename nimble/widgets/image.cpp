#pragma once
namespace Widgets {
    inline Widget Image(std::string path, Vector2 size, ObjectFit fit = ObjectFit::Cover) {
        return {"", size, {}, {}, {}, [path, fit](SDL_Renderer* renderer, SDL_Rect rect, const WidgetStyle&, const InputState&, const std::vector<Widget>&, WidgetDebug dbg) {
            static std::unordered_map<std::string, SDL_Texture*> cache;
            if(cache.find(path) == cache.end()) {
                SDL_Surface* surf = IMG_Load(path.c_str());
                if(surf) { 
                    cache[path] = SDL_CreateTextureFromSurface(renderer, surf); 
                    SDL_SetTextureScaleMode(cache[path], SDL_ScaleModeLinear);
                    SDL_FreeSurface(surf); 
                }
                else { cache[path] = nullptr; }
            }
            SDL_Texture* tex = cache[path];
            if(!tex) {
                SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                SDL_RenderFillRect(renderer, &rect);
                return;
            }

            int tw, th; SDL_QueryTexture(tex, NULL, NULL, &tw, &th);
            SDL_Rect src = {0, 0, tw, th};
            
            if (fit == ObjectFit::Cover) {
                float scale = std::max((float)rect.w / tw, (float)rect.h / th);
                src.w = rect.w / scale; src.h = rect.h / scale;
                src.x = (tw - src.w) / 2; src.y = (th - src.h) / 2;
            } else if (fit == ObjectFit::Contain) {
                float scale = std::min((float)rect.w / tw, (float)rect.h / th);
                int finalW = tw * scale; int finalH = th * scale;
                rect.x += (rect.w - finalW) / 2; rect.y += (rect.h - finalH) / 2;
                rect.w = finalW; rect.h = finalH;
            } else if (fit == ObjectFit::ScaleDown) {
                 float scale = std::min(1.0f, std::min((float)rect.w / tw, (float)rect.h / th));
                 int finalW = tw * scale; int finalH = th * scale;
                 rect.x += (rect.w - finalW) / 2; rect.y += (rect.h - finalH) / 2;
                 rect.w = finalW; rect.h = finalH;
            }
            SDL_RenderCopy(renderer, tex, &src, &rect);
        }};
    }
}