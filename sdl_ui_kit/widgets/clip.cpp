namespace Widgets {
    inline Widget Clip(const std::string& id, Vector2 size, int radius, Widget child = {}) {
        Widget w = {id, size};
        w.style.radius = radius;

        w.paint = [radius, id](SDL_Renderer* renderer, SDL_Rect rect, const WidgetStyle& s, 
                               const InputState& input, const std::vector<Widget>& children, WidgetDebug dbg) {
            
            if (children.empty() || rect.w <= 0 || rect.h <= 0) return;

// ... inside Clip's paint lambda ...

            SDL_Texture* targetTex = GetClipTexture(renderer, rect.w, rect.h, id + "_target");
            SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);
            
            SDL_SetRenderTarget(renderer, targetTex);
            
            // CRITICAL FIX: Clear with transparent WHITE, not transparent black!
            // This prevents dark gray fringes on alpha-blended text.
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0); 
            SDL_RenderClear(renderer); 

            // ... rest of the Clip logic ...
            // CRITICAL FIX: Use the child's true size, NOT the clipping rect's size!
            // This prevents the TextField from squishing and glitching out.
            int childW = children[0].size.x > 0 ? children[0].size.x : rect.w;
            int childH = children[0].size.y > 0 ? children[0].size.y : rect.h;

            SDL_Rect childTargetRect = {0, 0, childW, childH};
            children[0].render(renderer, childTargetRect, input, dbg);

            if (radius > 0) {
                SDL_Texture* maskTex = GetClipTexture(renderer, rect.w, rect.h, id + "_mask");
                SDL_SetRenderTarget(renderer, maskTex);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
                SDL_RenderClear(renderer);

                SDL_SetRenderTarget(renderer, targetTex);
                SDL_BlendMode maskMode = SDL_ComposeCustomBlendMode(
                    SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDOPERATION_ADD,
                    SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDOPERATION_ADD
                );
                SDL_SetTextureBlendMode(maskTex, maskMode);
                SDL_RenderCopy(renderer, maskTex, nullptr, nullptr);
            }

            SDL_SetRenderTarget(renderer, oldTarget);
            SDL_SetTextureBlendMode(targetTex, SDL_BLENDMODE_BLEND);
            SDL_RenderCopy(renderer, targetTex, nullptr, &rect);
        };

        if (child.paint) w.children.push_back(std::move(child));
        return w;
    }
}