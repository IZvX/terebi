// bgblur.cpp
#pragma once
namespace Widgets {
    inline Widget BackdropBlur(int blurRadius, Widget child) {
        return {"", child.size, {}, {}, {child}, [blurRadius](SDL_Renderer* renderer, SDL_Rect rect, const WidgetStyle&, const InputState& input, const std::vector<Widget>& children, WidgetDebug dbg) {
            if (blurRadius <= 0) {
                if(!children.empty()) children[0].render(renderer, rect, input, dbg);
                return;
            }
            if (rect.w <= 0 || rect.h <= 0) return;

            // Capture Background Segment
            SDL_Surface* screen = SDL_CreateRGBSurfaceWithFormat(0, rect.w, rect.h, 32, SDL_PIXELFORMAT_ABGR8888);
            SDL_RenderReadPixels(renderer, &rect, SDL_PIXELFORMAT_ABGR8888, screen->pixels, screen->pitch);

            SDL_Texture* screenTex = SDL_CreateTextureFromSurface(renderer, screen);
            SDL_FreeSurface(screen);

            // Fast GPU Path
            RenderGPUGaussian(renderer, screenTex, rect.w, rect.h, std::min(blurRadius, 20), rect);
            SDL_DestroyTexture(screenTex);

            if(!children.empty()) {
                children[0].render(renderer, rect, input, dbg);
            }
        }};
    }
}