// fgblur.cpp
#pragma once
namespace Widgets {
    inline Widget ForegroundBlur(int blurRadius, Widget child) {
         return {"", child.size, {}, {}, {child}, [blurRadius](SDL_Renderer* renderer, SDL_Rect rect, const WidgetStyle&, const InputState& input, const std::vector<Widget>& children, WidgetDebug dbg) {
             if (blurRadius <= 0) {
                 if(!children.empty()) children[0].render(renderer, rect, input, dbg);
                 return;
             }
             
             InputState localInput = input;
             localInput.mouseX -= rect.x;
             localInput.mouseY -= rect.y;

             SDL_Texture* parentTarget = SDL_GetRenderTarget(renderer);

             SDL_Texture* target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, rect.w, rect.h);
             SDL_SetTextureBlendMode(target, SDL_BLENDMODE_BLEND);
             SDL_SetRenderTarget(renderer, target);
             SDL_SetRenderDrawColor(renderer, 0,0,0,0); 
             SDL_RenderClear(renderer);
             
             if(!children.empty()) {
                 SDL_Rect localRect = {0,0,rect.w,rect.h};
                 children[0].render(renderer, localRect, localInput, dbg);
             }

             // Restore parent target BEFORE blurring!
             SDL_SetRenderTarget(renderer, parentTarget);
             
             // Fast GPU path (skips CPU ReadPixels entirely)
             RenderGPUGaussian(renderer, target, rect.w, rect.h, blurRadius, rect);

             SDL_DestroyTexture(target);
         }};
    }
}