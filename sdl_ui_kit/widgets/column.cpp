#pragma once
namespace Widgets {
    inline Widget Column(MainAxisAlignment mainAlign, CrossAxisAlignment crossAlign, int spacing, const std::vector<Widget> &children, ScrollBehavior scroll = ScrollBehavior::None, ScrollAxis axis = ScrollAxis::Vertical, std::string id = "") {
        int fixedH = 0, maxW = 0;
        for (const auto &c : children) { 
            if (c.expandY <= 0.0f) fixedH += c.size.y;
            maxW = std::max(maxW, c.size.x); 
        }
        if (!children.empty()) fixedH += spacing * (children.size() - 1);

        return {"", {maxW, fixedH}, {}, {}, children,[mainAlign, crossAlign, spacing, fixedW=maxW, fixedH, scroll, axis, id]
            (SDL_Renderer *r, SDL_Rect rect, const WidgetStyle& st, const InputState& in, const std::vector<Widget>& childs, WidgetDebug dbg) {
            
            int flexSpaceY = std::max(0, rect.h - fixedH);
            int actualTotalH = fixedH;
            for (const auto &c : childs) if (c.expandY > 0) actualTotalH += (int)(flexSpaceY * c.expandY);

            int actualTotalW = fixedW;

            bool canScrollX = (scroll != ScrollBehavior::None) && (axis == ScrollAxis::Horizontal || axis == ScrollAxis::Both);
            bool canScrollY = (scroll != ScrollBehavior::None) && (axis == ScrollAxis::Vertical || axis == ScrollAxis::Both);

            int maxScrollX = canScrollX ? std::max(0, actualTotalW - rect.w) : 0;
            int maxScrollY = canScrollY ? std::max(0, actualTotalH - rect.h) : 0;

            std::string sId = id.empty() ? ("scr_col_" + std::to_string(rect.x) + "_" + std::to_string(rect.y)) : id;
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
            int remainingSpace = rect.h - actualTotalH;
            int stepGap = spacing;

            if (remainingSpace > 0) {
                if(mainAlign == MainAxisAlignment::Center) currentY += remainingSpace / 2;
                else if(mainAlign == MainAxisAlignment::End) currentY += remainingSpace;
                else if(mainAlign == MainAxisAlignment::SpaceBetween && childs.size() > 1) stepGap += remainingSpace / (childs.size() - 1);
                else if(mainAlign == MainAxisAlignment::SpaceEvenly) { stepGap += remainingSpace / (childs.size() + 1); currentY += stepGap - spacing; }
            }

            for (size_t i = 0; i < childs.size(); ++i) {
                const auto& c = childs[i];
                int cw = (c.expandX > 0) ? (int)(rect.w * c.expandX) : c.size.x;
                int ch = (c.expandY > 0) ? (int)(flexSpaceY * c.expandY) : c.size.y;

                // CRUCIAL CONSTRAINT: Bound cross-axis so inner nested scrollable widgets realize they need to shrink!
                if (!canScrollX) cw = std::min(cw, rect.w);

                SDL_Rect cRect = {currentX, currentY, cw, ch};
                if (crossAlign == CrossAxisAlignment::Center) cRect.x += (rect.w - cRect.w) / 2;
                else if (crossAlign == CrossAxisAlignment::End) cRect.x += (rect.w - cRect.w);
                else if (crossAlign == CrossAxisAlignment::Stretch) { cRect.x = currentX; cRect.w = canScrollX ? std::max(rect.w, actualTotalW) : rect.w; }

                c.render(r, cRect, in, dbg);
                currentY += cRect.h;

                if (dbg.enabled && dbg.showSpacing && i < childs.size() - 1) {
                    SDL_Rect spaceRect = {currentX, currentY, rect.w, stepGap};
                    DebugDraw::DrawDiagonalRect(r, spaceRect, dbg.spacingColor);
                }
                currentY += stepGap;
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