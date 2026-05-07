#pragma once
namespace Widgets {
    inline Widget Icon(IconData icon, TTF_Font* font, int size, SDL_Color color, Gradient grad = {}, int thickness = 0) {
        std::string utf8_str; uint32_t cp = icon.codepoint;
        if (cp <= 0x7F) { utf8_str += (char)cp; }
        else if (cp <= 0x7FF) { utf8_str += (char)(0xC0 | (cp >> 6)); utf8_str += (char)(0x80 | (cp & 0x3F)); }
        else if (cp <= 0xFFFF) { utf8_str += (char)(0xE0 | (cp >> 12)); utf8_str += (char)(0x80 | ((cp >> 6) & 0x3F)); utf8_str += (char)(0x80 | (cp & 0x3F)); }
        WidgetStyle s; s.color = color; s.gradient = grad;
        return {"", {size, size}, s, {}, {}, [utf8_str, font, size, thickness](SDL_Renderer *r, SDL_Rect rect, const WidgetStyle& st, const InputState&, const std::vector<Widget>&, WidgetDebug dbg) {
            if (!font) return;
            SDL_Surface* surf = TTF_RenderUTF8_Blended(font, utf8_str.c_str(), st.color);
            if (surf) {
                if(st.gradient.enabled) ApplySurfaceGradient(surf, st.gradient);
                SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
                if (tex) {
                    SDL_SetTextureScaleMode(tex, SDL_ScaleModeLinear);
                    float scale = std::min((float)size/surf->w, (float)size/surf->h);
                    SDL_Rect iRect = {rect.x + (rect.w - (int)(surf->w*scale))/2, rect.y + (rect.h - (int)(surf->h*scale))/2, (int)(surf->w*scale), (int)(surf->h*scale)};
                    
                    if (thickness > 0) {
                        SDL_SetTextureAlphaMod(tex, 150); 
                        for (float angle = 0; angle < 2 * M_PI; angle += M_PI / 4) {
                            SDL_Rect thickRect = {iRect.x + (int)(std::cos(angle) * thickness), iRect.y + (int)(std::sin(angle) * thickness), iRect.w, iRect.h};
                            SDL_RenderCopy(r, tex, NULL, &thickRect);
                        }
                        SDL_SetTextureAlphaMod(tex, 255);
                    }
                    SDL_RenderCopy(r, tex, NULL, &iRect);
                    SDL_DestroyTexture(tex);
                }
                SDL_FreeSurface(surf);
            }
        }};
    }
}

static std::string CodepointToUTF8(uint32_t cp)
{
    std::string out;

    if (cp <= 0x7F)
        out += (char)cp;
    else if (cp <= 0x7FF)
    {
        out += (char)(0xC0 | (cp >> 6));
        out += (char)(0x80 | (cp & 0x3F));
    }
    else if (cp <= 0xFFFF)
    {
        out += (char)(0xE0 | (cp >> 12));
        out += (char)(0x80 | ((cp >> 6) & 0x3F));
        out += (char)(0x80 | (cp & 0x3F));
    }
    else
    {
        out += (char)(0xF0 | (cp >> 18));
        out += (char)(0x80 | ((cp >> 12) & 0x3F));
        out += (char)(0x80 | ((cp >> 6) & 0x3F));
        out += (char)(0x80 | (cp & 0x3F));
    }

    return out;
}