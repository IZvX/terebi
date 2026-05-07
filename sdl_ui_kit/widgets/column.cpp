#pragma once
#include <numeric>
#include <algorithm>

namespace Widgets {
    inline Widget Column(MainAxisAlignment mainAlign, CrossAxisAlignment crossAlign, int spacing, const std::vector<Widget> &children, ScrollBehavior scroll = ScrollBehavior::None, ScrollAxis axis = ScrollAxis::Vertical, std::string id = "") {
        int intrinsicH = 0, intrinsicW = 0;
        for (const auto &c : children) {
            if (c.expandY <= 0.0f) intrinsicH += c.size.y;
            if (c.expandX <= 0.0f) intrinsicW = std::max(intrinsicW, c.size.x); // FIX: skip expandX children in intrinsic width
        }
        if (!children.empty()) intrinsicH += spacing * (int)(children.size() - 1);

        return {"", {intrinsicW, intrinsicH}, {}, {}, children, [mainAlign, crossAlign, spacing, scroll, axis, id]
            (SDL_Renderer *r, SDL_Rect rect, const WidgetStyle& st, const InputState& in, const std::vector<Widget>& childs, WidgetDebug dbg) {

            if (rect.w <= 0 || rect.h <= 0) return;

            int actualFixedH = 0;
            float totalFlexY = 0.0f;
            int actualTotalW = 0;

            for (const auto &c : childs) {
                if (c.expandY > 0.0f) totalFlexY += c.expandY;
                else actualFixedH += c.size.y;

                int childW = (c.expandX > 0.0f) ? (int)(rect.w * c.expandX) : c.size.x;
                actualTotalW = std::max(actualTotalW, childW);
            }
            if (!childs.empty()) actualFixedH += spacing * (int)(childs.size() - 1);

            int flexSpaceY = std::max(0, rect.h - actualFixedH);
            int actualTotalH = actualFixedH + (totalFlexY > 0.0f ? flexSpaceY : 0);

            bool canScrollX = (scroll != ScrollBehavior::None) && (axis == ScrollAxis::Horizontal || axis == ScrollAxis::Both);
            bool canScrollY = (scroll != ScrollBehavior::None) && (axis == ScrollAxis::Vertical   || axis == ScrollAxis::Both);

            int maxScrollX = canScrollX ? std::max(0, actualTotalW - rect.w) : 0;
            int maxScrollY = canScrollY ? std::max(0, actualTotalH - rect.h) : 0;

            std::string sId = id.empty() ? ("scr_col_" + std::to_string(rect.x) + "_" + std::to_string(rect.y)) : id;
            ScrollState& scr = g_ScrollState[sId];

            bool isHovered = (in.mouseX >= rect.x && in.mouseX <= rect.x + rect.w &&
                              in.mouseY >= rect.y && in.mouseY <= rect.y + rect.h);

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
            int stepGap = spacing;

            if (flexSpaceY > 0 && totalFlexY == 0.0f) {
                if      (mainAlign == MainAxisAlignment::Center)                             currentY += flexSpaceY / 2;
                else if (mainAlign == MainAxisAlignment::End)                                currentY += flexSpaceY;
                else if (mainAlign == MainAxisAlignment::SpaceBetween && childs.size() > 1) stepGap += flexSpaceY / (int)(childs.size() - 1);
                else if (mainAlign == MainAxisAlignment::SpaceEvenly) { stepGap += flexSpaceY / (int)(childs.size() + 1); currentY += stepGap - spacing; }
            }

            for (size_t i = 0; i < childs.size(); ++i) {
                const auto& c = childs[i];

                int ch = c.size.y;
                if (c.expandY > 0.0f) {
                    ch = (totalFlexY > 0.0f) ? (int)(flexSpaceY * (c.expandY / totalFlexY)) : 0;
                }

                int cw = c.size.x;
                if (crossAlign == CrossAxisAlignment::Stretch) {
                    cw = canScrollX ? std::max(rect.w, actualTotalW) : rect.w;
                } else if (c.expandX > 0.0f) {
                    cw = (int)(rect.w * c.expandX);
                }
                if (!canScrollX) cw = std::min(cw, rect.w);

                int cx = currentX;
                if      (crossAlign == CrossAxisAlignment::Center) cx += (rect.w - cw) / 2;
                else if (crossAlign == CrossAxisAlignment::End)    cx += (rect.w - cw);

                SDL_Rect cRect = {cx, currentY, cw, ch};
                c.render(r, cRect, in, dbg);
                currentY += ch + stepGap;

                if (dbg.enabled && dbg.showSpacing && i < childs.size() - 1) {
                    SDL_Rect spaceRect = {currentX, currentY - stepGap, rect.w, stepGap};
                    DebugDraw::DrawDiagonalRect(r, spaceRect, dbg.spacingColor);
                }
            }

            if (hasClip) SDL_RenderSetClipRect(r, &prevClip);
            else          SDL_RenderSetClipRect(r, NULL);

            // Scrollbars
            const ScrollbarStyle& sb = st.scrollbarStyle;
            if (scroll != ScrollBehavior::None && !sb.hidden) {
                bool always = (scroll == ScrollBehavior::Always);
                bool showX = canScrollX && maxScrollX > 0 && (always || isHovered || scr.scrollX > 0);
                bool showY = canScrollY && maxScrollY > 0 && (always || isHovered || scr.scrollY > 0);

                int trackX_W = rect.w - 4;
                int trackY_H = rect.h - 4;
                if (showX && showY) { trackX_W -= sb.thickness; trackY_H -= sb.thickness; }

                if (showX) {
                    int trackH = sb.thickness;
                    int trackY = rect.y + rect.h - trackH - 2;
                    SDL_Rect trackRect = {rect.x + 2, trackY, trackX_W, trackH};

                    float visibleRatio = actualTotalW > 0 ? std::clamp((float)rect.w / actualTotalW, 0.1f, 1.0f) : 1.0f;
                    int thumbW = std::max(20, (int)(trackRect.w * visibleRatio));
                    int thumbX = trackRect.x + (int)((scr.scrollX / maxScrollX) * (trackRect.w - thumbW));
                    SDL_Rect thumbRect = {thumbX, trackY, thumbW, trackH};

                    bool thumbHovered = (in.mouseX >= thumbRect.x && in.mouseX <= thumbRect.x + thumbRect.w &&
                                         in.mouseY >= thumbRect.y && in.mouseY <= thumbRect.y + thumbRect.h);
                    FillRoundedBoxAA(r, trackRect, sb.radius, sb.trackColor);
                    FillRoundedBoxAA(r, thumbRect, sb.radius, thumbHovered ? sb.thumbHoverColor : sb.thumbColor);
                }

                if (showY) {
                    int trackW = sb.thickness;
                    int trackX = rect.x + rect.w - trackW - 2;
                    SDL_Rect trackRect = {trackX, rect.y + 2, trackW, trackY_H};

                    float visibleRatio = actualTotalH > 0 ? std::clamp((float)rect.h / actualTotalH, 0.1f, 1.0f) : 1.0f;
                    int thumbH = std::max(20, (int)(trackRect.h * visibleRatio));
                    int thumbY = trackRect.y + (int)((scr.scrollY / maxScrollY) * (trackRect.h - thumbH));
                    SDL_Rect thumbRect = {trackX, thumbY, trackW, thumbH};

                    bool thumbHovered = (in.mouseX >= thumbRect.x && in.mouseX <= thumbRect.x + thumbRect.w &&
                                         in.mouseY >= thumbRect.y && in.mouseY <= thumbRect.y + thumbRect.h);
                    FillRoundedBoxAA(r, trackRect, sb.radius, sb.trackColor);
                    FillRoundedBoxAA(r, thumbRect, sb.radius, thumbHovered ? sb.thumbHoverColor : sb.thumbColor);
                }
            }
        }};
    }
}