#pragma once

#define SDL_ENABLE_OLD_NAMES
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <climits>
#include <cstring>

inline SDL_FColor SDL_ColorToFColor(SDL_Color color)
{
    return {
        color.r / 255.0f,
        color.g / 255.0f,
        color.b / 255.0f,
        color.a / 255.0f
    };
}

inline bool TTF_SizeUTF8(TTF_Font *font, const char *text, int *w, int *h)
{
    const char *value = text ? text : "";
    return TTF_GetStringSize(font, value, std::strlen(value), w, h);
}

inline int TTF_FontHeight(TTF_Font *font)
{
    return TTF_GetFontHeight(font);
}

inline SDL_Surface *TTF_RenderUTF8_Blended(TTF_Font *font, const char *text, SDL_Color fg)
{
    return TTF_RenderText_Blended(font, text ? text : "", std::strlen(text ? text : ""), fg);
}

inline bool SDL_RenderSetClipRectCompat(SDL_Renderer *renderer, const SDL_Rect *rect)
{
    return SDL_SetRenderClipRect(renderer, rect);
}

inline void SDL_RenderGetClipRectCompat(SDL_Renderer *renderer, SDL_Rect *rect)
{
    SDL_GetRenderClipRect(renderer, rect);
}

inline int SDL_QueryTexture(SDL_Texture *texture, Uint32 *format, int *access, int *w, int *h)
{
    float fw = 0.0f;
    float fh = 0.0f;
    if (!SDL_GetTextureSize(texture, &fw, &fh))
        return -1;

    if (w)
        *w = static_cast<int>(fw);
    if (h)
        *h = static_cast<int>(fh);
    if (format)
        *format = 0;
    if (access)
        *access = 0;
    return 0;
}

inline bool SDL_RenderCopyRectCompat(SDL_Renderer *renderer, SDL_Texture *texture,
                                     const SDL_Rect *srcrect, const SDL_Rect *dstrect)
{
    SDL_FRect src{};
    SDL_FRect dst{};
    const SDL_FRect *psrc = nullptr;
    const SDL_FRect *pdst = nullptr;

    if (srcrect)
    {
        src = {(float)srcrect->x, (float)srcrect->y, (float)srcrect->w, (float)srcrect->h};
        psrc = &src;
    }
    if (dstrect)
    {
        dst = {(float)dstrect->x, (float)dstrect->y, (float)dstrect->w, (float)dstrect->h};
        pdst = &dst;
    }

    return SDL_RenderTexture(renderer, texture, psrc, pdst);
}

inline bool SDL_RenderFillRectCompat(SDL_Renderer *renderer, const SDL_Rect *rect)
{
    if (!rect)
        return SDL_RenderFillRect(renderer, nullptr);

    SDL_FRect frect = {(float)rect->x, (float)rect->y, (float)rect->w, (float)rect->h};
    return SDL_RenderFillRect(renderer, &frect);
}

inline bool SDL_RenderDrawRectCompat(SDL_Renderer *renderer, const SDL_Rect *rect)
{
    if (!rect)
        return SDL_RenderRect(renderer, nullptr);

    SDL_FRect frect = {(float)rect->x, (float)rect->y, (float)rect->w, (float)rect->h};
    return SDL_RenderRect(renderer, &frect);
}

inline bool SDL_RenderDrawLineCompat(SDL_Renderer *renderer, int x1, int y1, int x2, int y2)
{
    return SDL_RenderLine(renderer, (float)x1, (float)y1, (float)x2, (float)y2);
}

inline bool SDL_IntersectRectCompat(const SDL_Rect *a, const SDL_Rect *b, SDL_Rect *out)
{
    return SDL_GetRectIntersection(a, b, out);
}

#undef SDL_RenderCopy
#define SDL_RenderCopy(renderer, texture, srcrect, dstrect) \
    SDL_RenderCopyRectCompat(renderer, texture, srcrect, dstrect)

#undef SDL_RenderCopyF
#define SDL_RenderCopyF(renderer, texture, srcrect, dstrect) \
    SDL_RenderTexture(renderer, texture, srcrect, dstrect)

#undef SDL_RenderFillRect
#define SDL_RenderFillRect(renderer, rect) SDL_RenderFillRectCompat(renderer, rect)

#undef SDL_RenderDrawRect
#define SDL_RenderDrawRect(renderer, rect) SDL_RenderDrawRectCompat(renderer, rect)

#undef SDL_RenderDrawLine
#define SDL_RenderDrawLine(renderer, x1, y1, x2, y2) \
    SDL_RenderDrawLineCompat(renderer, x1, y1, x2, y2)

#undef SDL_RenderSetClipRect
#define SDL_RenderSetClipRect(renderer, rect) SDL_RenderSetClipRectCompat(renderer, rect)

#undef SDL_RenderGetClipRect
#define SDL_RenderGetClipRect(renderer, rect) SDL_RenderGetClipRectCompat(renderer, rect)

#undef SDL_RenderIsClipEnabled
#define SDL_RenderIsClipEnabled(renderer) SDL_RenderClipEnabled(renderer)

#undef SDL_IntersectRect
#define SDL_IntersectRect(a, b, out) SDL_IntersectRectCompat(a, b, out)

#undef SDL_FreeSurface
#define SDL_FreeSurface(surface) SDL_DestroySurface(surface)
