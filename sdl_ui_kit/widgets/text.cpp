#pragma once
namespace Widgets {
    inline Widget Text(const std::string& text, TTF_Font* font, SDL_Color color, Gradient grad = {}) {
        int w = 0, h = 0; if (font && !text.empty()) TTF_SizeUTF8(font, text.c_str(), &w, &h);
        WidgetStyle s; s.color = color; s.gradient = grad;
        return {"", {w, h}, s, {}, {},[text, font](SDL_Renderer *r, SDL_Rect rect, const WidgetStyle& st, const InputState&, const std::vector<Widget>&, WidgetDebug dbg) {
            if (!font || text.empty()) return;
            SDL_Surface* surf = TTF_RenderUTF8_Blended(font, text.c_str(), st.color);
            if (surf) {
                if(st.gradient.enabled) ApplySurfaceGradient(surf, st.gradient);
                SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
                if (tex) { 
                    SDL_SetTextureScaleMode(tex, SDL_ScaleModeLinear);
                    
                    // Render exactly at surface size, aligned to the left and centered vertically
                    SDL_Rect targetRect = {rect.x, rect.y + (rect.h - surf->h) / 2, surf->w, surf->h};
                    SDL_RenderCopy(r, tex, NULL, &targetRect); 
                    
                    SDL_DestroyTexture(tex); 
                }
                SDL_FreeSurface(surf);
            }
        }};
    }
}