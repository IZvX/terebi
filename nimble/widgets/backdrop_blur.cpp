// bgblur.cpp
#pragma once
namespace Widgets {
    inline Widget BackdropBlur(int blurRadius, Widget child) {
        return {"", child.size, {}, {}, {child}, [blurRadius](SDL_Renderer* renderer, SDL_Rect rect, const WidgetStyle&, const InputState& input, const std::vector<Widget>& children, WidgetDebug dbg) {
            if (blurRadius <= 0) {
                if(!children.empty()) children[0].render(renderer, rect, input, dbg);
                return;
            }

            struct BackdropBlurCache {
                SDL_Texture* blurred = nullptr;
                SDL_Rect rect = {0, 0, 0, 0};
                int radius = 0;
            };
            static BackdropBlurCache cache;

            const bool contentLikelyChanged =
                input.mouseClicked || input.rightMouseClicked ||
                input.leftMouseDown || input.rightMouseDown ||
                input.mouseWheelX != 0.0f || input.mouseWheelY != 0.0f ||
                input.keyPressed != SDLK_UNKNOWN || !input.textInput.empty();

            const bool cacheInvalid =
                !cache.blurred ||
                cache.radius != blurRadius ||
                cache.rect.w != rect.w || cache.rect.h != rect.h ||
                cache.rect.x != rect.x || cache.rect.y != rect.y ||
                contentLikelyChanged;

            if (cacheInvalid)
            {
                if (cache.blurred)
                {
                    SDL_DestroyTexture(cache.blurred);
                    cache.blurred = nullptr;
                }

                // SDL2 -> SDL3 migration: SDL_RenderReadPixels now returns a surface directly.
                SDL_Surface* screen = SDL_RenderReadPixels(renderer, &rect);
                if (!screen) {
                    if(!children.empty()) children[0].render(renderer, rect, input, dbg);
                    return;
                }

                // Upload directly to GPU instead of slow CPU loops
                SDL_Texture* screenTex = SDL_CreateTextureFromSurface(renderer, screen);
                SDL_FreeSurface(screen);
                if (!screenTex) {
                    if(!children.empty()) children[0].render(renderer, rect, input, dbg);
                    return;
                }

                cache.blurred = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, rect.w, rect.h);
                if (!cache.blurred)
                {
                    SDL_DestroyTexture(screenTex);
                    if(!children.empty()) children[0].render(renderer, rect, input, dbg);
                    return;
                }
                SDL_SetTextureBlendMode(cache.blurred, SDL_BLENDMODE_BLEND);

                SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);
                SDL_SetRenderTarget(renderer, cache.blurred);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
                SDL_RenderClear(renderer);
                SDL_Rect localDest = {0, 0, rect.w, rect.h};
                RenderGPUGaussian(renderer, screenTex, rect.w, rect.h, blurRadius, localDest);
                SDL_SetRenderTarget(renderer, oldTarget);
                SDL_DestroyTexture(screenTex);

                cache.rect = rect;
                cache.radius = blurRadius;
            }

            SDL_RenderCopy(renderer, cache.blurred, nullptr, &rect);

            if(!children.empty()) {
                children[0].render(renderer, rect, input, dbg);
            }
        }};
    }
}