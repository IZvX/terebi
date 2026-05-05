#pragma once
#include "../utils/helpers.h"
namespace Widgets {
    inline Widget Row(MainAxisAlignment mainAlign, CrossAxisAlignment crossAlign, int spacing, const std::vector<Widget> &children, ScrollBehavior scroll = ScrollBehavior::None, ScrollAxis axis = ScrollAxis::Horizontal, std::string id = "") {
        int fixedW = 0, maxH = 0;
        for (const auto &c : children) { 
            if (c.expandX <= 0.0f) fixedW += c.size.x;
            maxH = std::max(maxH, c.size.y); 
        }
        if (!children.empty()) fixedW += spacing * (children.size() - 1);

        return {"", {fixedW, maxH}, {}, {}, children,[mainAlign, crossAlign, spacing, fixedH=maxH, fixedW, scroll, axis, id]
            (SDL_Renderer *r, SDL_Rect rect, const WidgetStyle& st, const InputState& in, const std::vector<Widget>& childs, WidgetDebug dbg) {
            
            if (rect.w <= 0 || rect.h <= 0) return;
            
            int flexSpaceX = std::max(0, rect.w - fixedW);
            int actualTotalW = fixedW;
            for (const auto &c : childs) if (c.expandX > 0) actualTotalW += (int)(flexSpaceX * c.expandX);
            
            int actualTotalH = fixedH;

            bool canScrollX = (scroll != ScrollBehavior::None) && (axis == ScrollAxis::Horizontal || axis == ScrollAxis::Both);
            bool canScrollY = (scroll != ScrollBehavior::None) && (axis == ScrollAxis::Vertical || axis == ScrollAxis::Both);

            int maxScrollX = canScrollX ? std::max(0, actualTotalW - rect.w) : 0;
            int maxScrollY = canScrollY ? std::max(0, actualTotalH - rect.h) : 0;

            std::string sId = id.empty() ? ("scr_row_" + std::to_string(rect.x) + "_" + std::to_string(rect.y)) : id;
            ScrollState& scr = g_ScrollState[sId];

            bool isHovered = (in.mouseX >= rect.x && in.mouseX <= rect.x + rect.w && in.mouseY >= rect.y && in.mouseY <= rect.y + rect.h);

            if (isHovered) {
                if (canScrollX && in.mouseWheelX != 0) scr.targetX -= in.mouseWheelX * 40.0f;
                if (canScrollY && in.mouseWheelY != 0) scr.targetY -= in.mouseWheelY * 40.0f;
                
                scr.targetX = std::clamp(scr.targetX, 0.0f, (float)maxScrollX);
                scr.targetY = std::clamp(scr.targetY, 0.0f, (float)maxScrollY);
            }
            if (maxScrollX == 0) scr.targetX = 0;
            if (maxScrollY == 0) scr.targetY = 0;

            SDL_Rect prevClip;
            bool hasClip = SDL_RenderIsClipEnabled(r);
            if (hasClip) SDL_RenderGetClipRect(r, &prevClip);
            SDL_Rect newClip = rect;
            if (hasClip) SDL_IntersectRect(&newClip, &prevClip, &newClip);
            SDL_RenderSetClipRect(r, &newClip);

            int currentX = rect.x - (int)scr.scrollX;
            int currentY = rect.y - (int)scr.scrollY;
            int remainingSpace = rect.w - actualTotalW;
            int stepGap = spacing;

            if (remainingSpace > 0) {
                if(mainAlign == MainAxisAlignment::Center) currentX += remainingSpace / 2;
                else if(mainAlign == MainAxisAlignment::End) currentX += remainingSpace;
                else if(mainAlign == MainAxisAlignment::SpaceBetween && childs.size() > 1) stepGap += remainingSpace / (childs.size() - 1);
                else if(mainAlign == MainAxisAlignment::SpaceEvenly) { stepGap += remainingSpace / (childs.size() + 1); currentX += stepGap - spacing; }
            }

            for (size_t i = 0; i < childs.size(); ++i) {
                const auto& c = childs[i];
                int cw = (c.expandX > 0) ? (int)(flexSpaceX * c.expandX) : c.size.x;
                int ch = (c.expandY > 0) ? (int)(rect.h * c.expandY) : c.size.y;

                // CRUCIAL CONSTRAINT: Bound cross-axis so inner nested scrollable widgets realize they need to shrink!
                if (!canScrollY) ch = std::min(ch, rect.h);

                SDL_Rect cRect = {currentX, currentY, cw, ch};
                if (crossAlign == CrossAxisAlignment::Center) cRect.y += (rect.h - cRect.h) / 2;
                else if (crossAlign == CrossAxisAlignment::End) cRect.y += (rect.h - cRect.h);
                else if (crossAlign == CrossAxisAlignment::Stretch) { cRect.y = currentY; cRect.h = canScrollY ? std::max(rect.h, actualTotalH) : rect.h; }

                c.render(r, cRect, in, dbg);
                currentX += cRect.w;

                if (dbg.enabled && dbg.showSpacing && i < childs.size() - 1) {
                    SDL_Rect spaceRect = {currentX, currentY, stepGap, rect.h};
                    DebugDraw::DrawDiagonalRect(r, spaceRect, dbg.spacingColor);
                }
                currentX += stepGap;
            }

            if (hasClip) SDL_RenderSetClipRect(r, &prevClip);
            else SDL_RenderSetClipRect(r, NULL);

            const ScrollbarStyle& sb = st.scrollbarStyle;
            if (scroll != ScrollBehavior::None && !sb.hidden) {
                bool always = (scroll == ScrollBehavior::Always);
                bool showX = canScrollX && maxScrollX > 0 && (always || isHovered || scr.scrollX > 0);
                bool showY = canScrollY && maxScrollY > 0 && (always || isHovered || scr.scrollY > 0);

                int trackX_W = rect.w - 4;
                int trackY_H = rect.h - 4;

                // Prevent overlap in corner if multi-axis scrollbars are both active
                if (showX && showY) {
                    trackX_W -= sb.thickness;
                    trackY_H -= sb.thickness;
                }

                if (showX) {
                    int trackH = sb.thickness;
                    int trackY = rect.y + rect.h - trackH - 2;
                    SDL_Rect trackRect = {rect.x + 2, trackY, trackX_W, trackH};

                    float visibleRatio = std::clamp((float)rect.w / actualTotalW, 0.1f, 1.0f);
                    int thumbW = std::max(20, (int)(trackRect.w * visibleRatio));
                    int thumbX = trackRect.x + (int)((scr.scrollX / maxScrollX) * (trackRect.w - thumbW));
                    SDL_Rect thumbRect = {thumbX, trackY, thumbW, trackH};

                    bool thumbHovered = (in.mouseX >= thumbRect.x && in.mouseX <= thumbRect.x + thumbRect.w && in.mouseY >= thumbRect.y && in.mouseY <= thumbRect.y + thumbRect.h);
                    SDL_Color tc = thumbHovered ? sb.thumbHoverColor : sb.thumbColor;

                    FillRoundedBoxAA(r, trackRect, sb.radius, sb.trackColor);
                    FillRoundedBoxAA(r, thumbRect, sb.radius, tc);
                }

                if (showY) {
                    int trackW = sb.thickness;
                    int trackX = rect.x + rect.w - trackW - 2;
                    SDL_Rect trackRect = {trackX, rect.y + 2, trackW, trackY_H};

                    float visibleRatio = std::clamp((float)rect.h / actualTotalH, 0.1f, 1.0f);
                    int thumbH = std::max(20, (int)(trackRect.h * visibleRatio));
                    int thumbY = trackRect.y + (int)((scr.scrollY / maxScrollY) * (trackRect.h - thumbH));
                    SDL_Rect thumbRect = {trackX, thumbY, trackW, thumbH};

                    bool thumbHovered = (in.mouseX >= thumbRect.x && in.mouseX <= thumbRect.x + thumbRect.w && in.mouseY >= thumbRect.y && in.mouseY <= thumbRect.y + thumbRect.h);
                    SDL_Color tc = thumbHovered ? sb.thumbHoverColor : sb.thumbColor;

                    FillRoundedBoxAA(r, trackRect, sb.radius, sb.trackColor);
                    FillRoundedBoxAA(r, thumbRect, sb.radius, tc);
                }
            }
        }};
    }
}