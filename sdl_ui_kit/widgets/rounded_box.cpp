#pragma once

namespace Widgets
{
    inline Widget RoundedBox(Vector2 size, WidgetStyle style, Widget child = {})
    {
        const bool shouldAutoSize = (size.x <= 0.0f || size.y <= 0.0f);

        Widget w = {"RoundedBox", size, style, {}, {}, [style, shouldAutoSize](SDL_Renderer *renderer, SDL_Rect rect, const WidgetStyle &s, const InputState &input, const std::vector<Widget> &childrenList, WidgetDebug dbg)
                    {
                        // 1. Shadow
                        if (s.shadowColor.a > 0)
                        {
                            SDL_Texture *shadowTex = GetShadowTexture(renderer, rect.w, rect.h,
                                                                      s.radius, s.shadowSpread, s.shadowBlur);
                            if (shadowTex)
                            {
                                SDL_SetTextureColorMod(shadowTex, s.shadowColor.r, s.shadowColor.g, s.shadowColor.b);
                                SDL_SetTextureAlphaMod(shadowTex, s.shadowColor.a);
                                int pad = s.shadowBlur + s.shadowSpread;
                                SDL_Rect shadowRect = {
                                    rect.x + s.shadowOffset.x - pad,
                                    rect.y + s.shadowOffset.y - pad,
                                    rect.w + pad * 2,
                                    rect.h + pad * 2};
                                SDL_RenderCopy(renderer, shadowTex, nullptr, &shadowRect);
                            }
                        }

                        // 2. Background
                        FillRoundedBoxAA(renderer, rect, s.radius, s.color, s.gradient);

                        // 3. Border (Draws outside of rect, totally hollow inside)
                        if (s.borderWidth > 0 && s.borderColor.a > 0)
                        {

                            // --- OVERFLOW: VISIBLE HACK ---
                            // Temporarily disable clipping so the border can safely bleed outside the 44x44 box!
                            SDL_Rect currentClip;
                            bool hasClip = SDL_RenderIsClipEnabled(renderer);
                            if (hasClip)
                            {
                                SDL_RenderGetClipRect(renderer, &currentClip);
                                SDL_RenderSetClipRect(renderer, nullptr); // Disable scissors
                            }

                            DrawRoundedBoxOutlineAA(renderer, rect, s.radius, s.borderWidth, s.borderColor);

                            // Restore scissors so other widgets don't break
                            if (hasClip)
                            {
                                SDL_RenderSetClipRect(renderer, &currentClip);
                            }
                        }

                        // Render child (centered)
                        if (!childrenList.empty())
                        {
                            const auto &c = childrenList[0];
                            int cw = c.size.x;
                            int ch = c.size.y;

                            if (c.expandX > 0.0f)
                                cw = (int)(rect.w * c.expandX);
                            else if (cw > rect.w)
                                cw = rect.w;

                            if (c.expandY > 0.0f)
                                ch = (int)(rect.h * c.expandY);
                            else if (ch > rect.h)
                                ch = rect.h;

                            SDL_Rect cRect = {
                                rect.x + (rect.w - cw) / 2,
                                rect.y + (rect.h - ch) / 2,
                                cw, ch};

                            c.render(renderer, cRect, input, dbg);
                        }
                    }};

        if (child.paint)
            w.children.push_back(std::move(child));

        if (shouldAutoSize && !w.children.empty())
        {
            const Widget &childWidget = w.children[0];
            Vector2 padding = style.padding;
            float border = style.borderWidth * 2.0f;
            w.size.x = childWidget.size.x + padding.x + border;
            w.size.y = childWidget.size.y + padding.y + border;
        }

        return w;
    }

    inline Widget Box(Vector2 size, WidgetStyle style, Widget child = {})
    {
        const bool shouldAutoSize = (size.x <= 0.0f || size.y <= 0.0f);

        Widget w = {"Box", size, style, {}, {}, [style, shouldAutoSize](SDL_Renderer *renderer, SDL_Rect rect, const WidgetStyle &s, const InputState &input, const std::vector<Widget> &childrenList, WidgetDebug dbg)
                    {
                        // 1. Shadow
                        if (s.shadowColor.a > 0)
                        {
                            SDL_Texture *shadowTex = GetShadowTexture(renderer, rect.w, rect.h,
                                                                      s.radius, s.shadowSpread, s.shadowBlur);
                            if (shadowTex)
                            {
                                SDL_SetTextureColorMod(shadowTex, s.shadowColor.r, s.shadowColor.g, s.shadowColor.b);
                                SDL_SetTextureAlphaMod(shadowTex, s.shadowColor.a);
                                int pad = s.shadowBlur + s.shadowSpread;
                                SDL_Rect shadowRect = {
                                    rect.x + s.shadowOffset.x - pad,
                                    rect.y + s.shadowOffset.y - pad,
                                    rect.w + pad * 2,
                                    rect.h + pad * 2};
                                SDL_RenderCopy(renderer, shadowTex, nullptr, &shadowRect);
                            }
                        }

                        // 2. Background (Stays exactly at rect)
                        FillBox(renderer, rect, s.color, s.gradient);

                        // 3. Border (Draws outside of rect, totally hollow inside)
                        if (s.borderWidth > 0 && s.borderColor.a > 0)
                        {
                            DrawBoxOutline(renderer, rect, s.borderWidth, s.borderColor);
                        }
                        // Render child (centered)
                        if (!childrenList.empty())
                        {
                            const auto &c = childrenList[0];
                            int cw = c.size.x;
                            int ch = c.size.y;

                            if (c.expandX > 0.0f)
                                cw = (int)(rect.w * c.expandX);
                            else if (cw > rect.w)
                                cw = rect.w;

                            if (c.expandY > 0.0f)
                                ch = (int)(rect.h * c.expandY);
                            else if (ch > rect.h)
                                ch = rect.h;

                            SDL_Rect cRect = {
                                rect.x + (rect.w - cw) / 2,
                                rect.y + (rect.h - ch) / 2,
                                cw, ch};

                            c.render(renderer, cRect, input, dbg);
                        }
                    }};

        if (child.paint)
            w.children.push_back(std::move(child));

        if (shouldAutoSize && !w.children.empty())
        {
            const Widget &childWidget = w.children[0];
            Vector2 padding = style.padding;
            float border = style.borderWidth * 2.0f;
            w.size.x = childWidget.size.x + padding.x + border;
            w.size.y = childWidget.size.y + padding.y + border;
        }

        return w;
    }
} // namespace Widgets