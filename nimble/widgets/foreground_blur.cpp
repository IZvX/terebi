// fgblur.cpp
#pragma once
namespace Widgets {
    inline Widget ForegroundBlur(int blurRadius, Widget child) {
         return {"", child.size, {}, {}, {child}, [blurRadius](SDL_Renderer* renderer, SDL_Rect rect, const WidgetStyle&, const InputState& input, const std::vector<Widget>& children, WidgetDebug dbg) {
             if (blurRadius <= 0) {
                 if(!children.empty()) children[0].render(renderer, rect, input, dbg);
                 return;
             }
            if (rect.w <= 0 || rect.h <= 0) return;
             
             InputState localInput = input;
             localInput.mouseX -= rect.x;
             localInput.mouseY -= rect.y;

             SDL_Texture* parentTarget = SDL_GetRenderTarget(renderer);

             SDL_Texture* target = GetClipTexture(renderer, rect.w, rect.h, "__fg_blur_target");
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
             RenderGPUGaussian(renderer, target, rect.w, rect.h, std::min(blurRadius, 20), rect);
         }};
    }
}