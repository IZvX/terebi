#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

// SDL2 -> SDL3 migration shim:
// keep legacy call sites while routing to SDL3 equivalents.

// SDL_image 3.x no longer requires IMG_Init/IMG_Quit for codec setup.
#ifndef IMG_INIT_PNG
#define IMG_INIT_PNG 0x00000001
#endif

static inline int SDL3Compat_IMG_Init(int /*flags*/) { return 1; }
static inline void SDL3Compat_IMG_Quit(void) {}

#undef IMG_Init
#undef IMG_Quit
#define IMG_Init SDL3Compat_IMG_Init
#define IMG_Quit SDL3Compat_IMG_Quit

static inline int SDL3Compat_TTF_SizeUTF8(TTF_Font *font, const char *text, int *w, int *h)
{
    if (!text) {
        if (w) *w = 0;
        if (h) *h = 0;
        return 0;
    }
    return TTF_GetStringSize(font, text, SDL_strlen(text), w, h) ? 0 : -1;
}

static inline SDL_Surface *SDL3Compat_TTF_RenderUTF8_Blended(TTF_Font *font, const char *text, SDL_Color fg)
{
    return TTF_RenderText_Blended(font, text, text ? SDL_strlen(text) : 0, fg);
}

#undef TTF_SizeUTF8
#undef TTF_RenderUTF8_Blended
#undef TTF_FontHeight
#define TTF_SizeUTF8 SDL3Compat_TTF_SizeUTF8
#define TTF_RenderUTF8_Blended SDL3Compat_TTF_RenderUTF8_Blended
#define TTF_FontHeight TTF_GetFontHeight

static inline bool SDL3Compat_RenderDrawRect(SDL_Renderer *renderer, const SDL_Rect *rect)
{
    if (!rect) return SDL_RenderRect(renderer, nullptr);
    const SDL_FRect frect = {(float)rect->x, (float)rect->y, (float)rect->w, (float)rect->h};
    return SDL_RenderRect(renderer, &frect);
}

static inline bool SDL3Compat_RenderFillRect(SDL_Renderer *renderer, const SDL_Rect *rect)
{
    if (!rect) return SDL_RenderFillRect(renderer, nullptr);
    const SDL_FRect frect = {(float)rect->x, (float)rect->y, (float)rect->w, (float)rect->h};
    return SDL_RenderFillRect(renderer, &frect);
}

static inline bool SDL3Compat_RenderCopy(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *src, const SDL_Rect *dst)
{
    SDL_FRect fsrc = {};
    SDL_FRect fdst = {};
    const SDL_FRect *src_ptr = nullptr;
    const SDL_FRect *dst_ptr = nullptr;

    if (src) {
        fsrc = {(float)src->x, (float)src->y, (float)src->w, (float)src->h};
        src_ptr = &fsrc;
    }
    if (dst) {
        fdst = {(float)dst->x, (float)dst->y, (float)dst->w, (float)dst->h};
        dst_ptr = &fdst;
    }

    return SDL_RenderTexture(renderer, texture, src_ptr, dst_ptr);
}

static inline int SDL3Compat_QueryTexture(SDL_Texture *texture, Uint32 *format, int *access, int *w, int *h)
{
    if (format) *format = 0;
    if (access) *access = 0;
    float fw = 0.0f, fh = 0.0f;
    if (!SDL_GetTextureSize(texture, &fw, &fh)) {
        if (w) *w = 0;
        if (h) *h = 0;
        return -1;
    }
    if (w) *w = (int)fw;
    if (h) *h = (int)fh;
    return 0;
}

#undef SDL_RenderDrawRect
#undef SDL_RenderFillRect
#undef SDL_RenderCopy
#undef SDL_QueryTexture
#undef SDL_FreeSurface

#define SDL_RenderDrawRect SDL3Compat_RenderDrawRect
#define SDL_RenderFillRect SDL3Compat_RenderFillRect
#define SDL_RenderCopy SDL3Compat_RenderCopy
#define SDL_QueryTexture SDL3Compat_QueryTexture
#define SDL_FreeSurface SDL_DestroySurface
