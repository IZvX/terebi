#pragma once
#include "../utils/helpers.h"

inline Widget Row(MainAxisAlignment mainAlign, CrossAxisAlignment crossAlign, int spacing, const std::vector<Widget> &children, ScrollBehavior scroll = ScrollBehavior::None, ScrollAxis axis = ScrollAxis::Horizontal, std::string id = "") {
    int intrinsicW = 0, intrinsicH = 0;
    for (const auto &c : children) {
        if (c.expandX <= 0.0f) intrinsicW += c.size.x;
        if (c.expandY <= 0.0f) intrinsicH = std::max(intrinsicH, c.size.y); // FIX: skip expandY children in intrinsic height
    }
    if (!children.empty()) intrinsicW += spacing * (int)(children.size() - 1);

    return {"", {intrinsicW, intrinsicH}, {}, {}, children, [mainAlign, crossAlign, spacing, scroll, axis, id]
        (SDL_Renderer *r, SDL_Rect rect, const WidgetStyle& st, const InputState& in, const std::vector<Widget>& childs, WidgetDebug dbg) {

        if (rect.w <= 0 || rect.h <= 0) return;

        int actualFixedW = 0;
        float totalFlexX = 0.0f;
        int actualTotalH = 0;

        for (const auto &c : childs) {
            if (c.expandX > 0.0f) totalFlexX += c.expandX;
            else actualFixedW += c.size.x;

            int childH = (c.expandY > 0.0f) ? (int)(rect.h * c.expandY) : c.size.y;
            actualTotalH = std::max(actualTotalH, childH);
        }
        if (!childs.empty()) actualFixedW += spacing * (int)(childs.size() - 1);

        int flexSpaceX = std::max(0, rect.w - actualFixedW);
        int actualTotalW = actualFixedW + (totalFlexX > 0.0f ? flexSpaceX : 0);

        bool canScrollX = (scroll != ScrollBehavior::None) && (axis == ScrollAxis::Horizontal || axis == ScrollAxis::Both);
        bool canScrollY = (scroll != ScrollBehavior::None) && (axis == ScrollAxis::Vertical   || axis == ScrollAxis::Both);

        int maxScrollX = canScrollX ? std::max(0, actualTotalW - rect.w) : 0;
        int maxScrollY = canScrollY ? std::max(0, actualTotalH - rect.h) : 0;

        std::string sId = id.empty() ? ("scr_row_" + std::to_string(rect.x) + "_" + std::to_string(rect.y)) : id;
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

        if (flexSpaceX > 0 && totalFlexX == 0.0f) {
            if      (mainAlign == MainAxisAlignment::Center)                           currentX += flexSpaceX / 2;
            else if (mainAlign == MainAxisAlignment::End)                              currentX += flexSpaceX;
            else if (mainAlign == MainAxisAlignment::SpaceBetween && childs.size() > 1) stepGap += flexSpaceX / (int)(childs.size() - 1);
            else if (mainAlign == MainAxisAlignment::SpaceEvenly) { stepGap += flexSpaceX / (int)(childs.size() + 1); currentX += stepGap - spacing; }
        }

        for (size_t i = 0; i < childs.size(); ++i) {
            const auto& c = childs[i];

            int cw = c.size.x;
            if (c.expandX > 0.0f) {
                cw = (totalFlexX > 0.0f) ? (int)(flexSpaceX * (c.expandX / totalFlexX)) : 0;
            }

            int ch = c.size.y;
            if (crossAlign == CrossAxisAlignment::Stretch) {
                ch = canScrollY ? std::max(rect.h, actualTotalH) : rect.h;
            } else if (c.expandY > 0.0f) {
                ch = (int)(rect.h * c.expandY);
            }
            if (!canScrollY) ch = std::min(ch, rect.h);

            int cy = currentY;
            if      (crossAlign == CrossAxisAlignment::Center) cy += (rect.h - ch) / 2;
            else if (crossAlign == CrossAxisAlignment::End)    cy += (rect.h - ch);

            SDL_Rect cRect = {currentX, cy, cw, ch};
            c.render(r, cRect, in, dbg);
            currentX += cw + stepGap;

            if (dbg.enabled && dbg.showSpacing && i < childs.size() - 1) {
                SDL_Rect spaceRect = {currentX - stepGap, currentY, stepGap, rect.h};
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