#pragma once
#include <string>
#include <algorithm>

namespace Widgets
{
    // Renders a child widget with a global opacity (0.0f to 1.0f)
    inline Widget Opacity(float alpha, Widget child = {}, std::string id = "Opacity")
    {
        // Clamp alpha to safe bounds
        alpha = std::max(0.0f, std::min(1.0f, alpha));
        Uint8 alpha8 = static_cast<Uint8>(alpha * 255.0f);

        Widget w = {id, {0, 0}, {}, {}, {}, [alpha8](SDL_Renderer *renderer, SDL_Rect rect, const WidgetStyle &s, const InputState &input, const std::vector<Widget> &childrenList, WidgetDebug dbg)
                    {
                        if (rect.w <= 0 || rect.h <= 0 || childrenList.empty()) return;

                        const auto &c = childrenList[0];

                        // 1. Cull rendering if completely invisible
                        if (alpha8 == 0) return;

                        // 2. Bypass texture overhead if completely opaque
                        if (alpha8 == 255)
                        {
                            c.render(renderer, rect, input, dbg);
                            return;
                        }

                        // 3. FLATTENING & OPACITY 
                        // Create a texture bounding the layout rectangle
                        SDL_Texture *target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, rect.w, rect.h);
                        
                        // Fallback if the hardware fails to allocate the render target
                        if (!target)
                        {
                            c.render(renderer, rect, input, dbg);
                            return;
                        }

                        SDL_SetTextureBlendMode(target, SDL_BLENDMODE_BLEND);

                        // Swap to our local texture
                        SDL_Texture *prevTarget = SDL_GetRenderTarget(renderer);
                        SDL_SetRenderTarget(renderer, target);

                        // Clear the texture with absolute transparency
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
                        SDL_RenderClear(renderer);
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

                        // The target origin is {0, 0}, so we need a localized layout rect
                        SDL_Rect localRect = {0, 0, rect.w, rect.h};
                        
                        // Offset the mouse interactions so hitboxes still match inside the localized texture
                        InputState localInput = input;
                        localInput.mouseX -= rect.x;
                        localInput.mouseY -= rect.y;

                        // Temporarily disable scissors/clipping. 
                        // Screen-space clip coordinates will break local-space target drawing.
                        SDL_Rect prevClip;
                        bool hasClip = SDL_RenderIsClipEnabled(renderer);
                        if (hasClip)
                        {
                            SDL_RenderGetClipRect(renderer, &prevClip);
                            SDL_RenderSetClipRect(renderer, nullptr);
                        }

                        // Render the child into the texture
                        c.render(renderer, localRect, localInput, dbg);

                        // Restore old clip boundaries
                        if (hasClip)
                        {
                            SDL_RenderSetClipRect(renderer, &prevClip);
                        }

                        // Restore original target (the window/parent texture)
                        SDL_SetRenderTarget(renderer, prevTarget);

                        // 4. Draw the flattened texture to the screen with the unified alpha modifier applied
                        SDL_SetTextureAlphaMod(target, alpha8);
                        SDL_RenderCopy(renderer, target, nullptr, &rect);

                        // Cleanup
                        SDL_DestroyTexture(target);
                    }};

        // Bind child
        if (child.paint)
            w.children.push_back(std::move(child));

        // Let the Opacity container accurately inherit size/expansion rules from its child
        if (!w.children.empty())
        {
            const Widget &childWidget = w.children[0];
            w.size = childWidget.size;
            w.expandX = childWidget.expandX;
            w.expandY = childWidget.expandY;
        }

        return w;
    }

    // Convenience Overload: Accept integer scale alpha (0 to 255)
    inline Widget Opacity(Uint8 alpha, Widget child = {}, std::string id = "Opacity")
    {
        return Opacity(alpha / 255.0f, std::move(child), id);
    }

} // namespace Widgets