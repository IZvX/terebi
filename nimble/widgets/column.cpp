#pragma once
#include <numeric>
#include <algorithm>

static bool WidgetContainsFocusedId(const Widget& w, const std::string& focusedId) {
    if (w.id == focusedId) return true;
    for (const auto& child : w.children)
        if (WidgetContainsFocusedId(child, focusedId)) return true;
    return false;
}


static Widget* FindNavWidget(Widget& w) {
    // Skip wrapper widgets that have no real id or a placeholder id
    if (!w.id.empty() && w.id != "Opacity" && w.id != "Expanded" && w.id != "Padding")
        return &w;
    for (auto& c : w.children)
        if (auto* found = FindNavWidget(c)) return found;
    return nullptr;
}

namespace Widgets {
    inline Widget Column(MainAxisAlignment mainAlign, CrossAxisAlignment crossAlign, int spacing, const std::vector<Widget> &children, ScrollBehavior scroll = ScrollBehavior::None, ScrollAxis axis = ScrollAxis::Vertical, std::string id = "",float expandX = 0.0f, float expandY = 0.0f,bool autoSetupNav = false) {
        // Intrinsic size: only used when a parent shrink-wraps this Column.
        int intrinsicH = 0, intrinsicW = 0;
        for (const auto &c : children) {
            if (c.expandY <= 0.0f) intrinsicH += c.size.y;
            if (c.expandX <= 0.0f) intrinsicW = std::max(intrinsicW, (int)c.size.x);
        }
        if (!children.empty()) intrinsicH += spacing * (int)(children.size() - 1);

        Widget w;
        w.size = {intrinsicW, intrinsicH};
        w.children = children;
        w.expandX = std::max(0.0f, expandX);
        w.expandY = std::max(0.0f, expandY);

        // ── Auto nav wiring ───────────────────────────────────────────────
            if (autoSetupNav) {
                std::vector<Widget*> navWidgets;
                for (auto& child : w.children) {
                    Widget* inner = FindNavWidget(child);
                    if (inner) navWidgets.push_back(inner);
                }
                for (size_t i = 0; i < navWidgets.size(); ++i) {
                    if (i > 0) {
                        navWidgets[i]->nav.up       = navWidgets[i-1]->id;
                        navWidgets[i-1]->nav.down   = navWidgets[i]->id;
                    }
                }
            }
        
        w.paint =[mainAlign, crossAlign, spacing, scroll, axis, id, autoSetupNav, w](SDL_Renderer *r, SDL_Rect rect, const WidgetStyle& st, const InputState& in, const std::vector<Widget>& childs, WidgetDebug dbg) mutable {            if (rect.w <= 0 || rect.h <= 0) return;

            int actualFixedH = 0;
            float totalFlexY = 0.0f;
            for (const auto &c : childs) {
                if (c.expandY > 0.0f) totalFlexY += c.expandY;
                else                  actualFixedH += c.size.y;
            }
            if (!childs.empty()) actualFixedH += spacing * (int)(childs.size() - 1);

            int flexSpaceY  = std::max(0, rect.h - actualFixedH);
            int actualTotalH = actualFixedH + (totalFlexY > 0.0f ? flexSpaceY : 0);

            int actualTotalW = rect.w;
            if (scroll != ScrollBehavior::None) {
                for (const auto &c : childs) {
                    int childW = (c.expandX > 0.0f) ? rect.w : c.size.x;
                    actualTotalW = std::max(actualTotalW, childW);
                }
            }

            bool canScrollX = (scroll != ScrollBehavior::None) && (axis == ScrollAxis::Horizontal || axis == ScrollAxis::Both);
            bool canScrollY = (scroll != ScrollBehavior::None) && (axis == ScrollAxis::Vertical   || axis == ScrollAxis::Both);

            int maxScrollX = canScrollX ? std::max(0, actualTotalW - rect.w) : 0;
            int maxScrollY = canScrollY ? std::max(0, actualTotalH - rect.h) : 0;

            std::string sId = id.empty() ? ("scr_col_" + std::to_string(rect.x) + "_" + std::to_string(rect.y)) : id;
            ScrollState& scr = g_ScrollState[sId];
            const float prevMaxScrollY = scr.lastMaxScrollY;
            const bool wasNearBottom = (prevMaxScrollY <= 1.0f) || (scr.targetY >= prevMaxScrollY - 2.0f);

            bool isHovered = (in.mouseX >= rect.x && in.mouseX <= rect.x + rect.w &&
                              in.mouseY >= rect.y && in.mouseY <= rect.y + rect.h);
            const bool userScrolledThisFrame = isHovered && canScrollY && (in.mouseWheelY != 0.0f);

            if (isHovered) {
                if (canScrollX && in.mouseWheelX != 0) scr.targetX -= in.mouseWheelX * 40.0f;
                if (canScrollY && in.mouseWheelY != 0) {
                    scr.targetY -= in.mouseWheelY * 40.0f;
                    // User scrolled upward: stop sticky auto-follow.
                    if (in.mouseWheelY > 0.0f) scr.stickToBottomY = false;
                }
                scr.targetX = std::clamp(scr.targetX, 0.0f, (float)maxScrollX);
                scr.targetY = std::clamp(scr.targetY, 0.0f, (float)maxScrollY);
            }
            if (maxScrollX == 0) scr.targetX = 0;
            if (maxScrollY == 0) scr.targetY = 0;

            // If content changes while user is at the end (or auto-follow is enabled),
            // stay pinned to bottom. Otherwise preserve current reading position.
            if (canScrollY && maxScrollY != (int)prevMaxScrollY) {
                if (scr.stickToBottomY || wasNearBottom) {
                    scr.targetY = (float)maxScrollY;
                    scr.stickToBottomY = true;
                } else {
                    scr.targetY = std::clamp(scr.targetY, 0.0f, (float)maxScrollY);
                }
            }

            // ── Auto-scroll to focused widget ────────────────────────────────────
            // Focus auto-scroll should only occur on focus transitions, not every frame.
            const bool focusChanged = (scr.lastFocusedWidgetId != g_FocusedWidgetId);
            scr.lastFocusedWidgetId = g_FocusedWidgetId;

            if (canScrollY && !g_FocusedWidgetId.empty() && focusChanged && !userScrolledThisFrame) {
                int scanY = 0;
                for (const auto& c : childs) {
                    int ch = c.size.y;
                    if (c.expandY > 0.0f)
                        ch = (totalFlexY > 0.0f) ? (int)(flexSpaceY * (c.expandY / totalFlexY)) : 0;

                    if (WidgetContainsFocusedId(c, g_FocusedWidgetId)) {
                        int childTop      = scanY;
                        int childBottom   = scanY + ch;
                        int visibleTop    = (int)scr.targetY;
                        int visibleBottom = (int)scr.targetY + rect.h;

                        if (childTop < visibleTop)
                            scr.targetY = (float)childTop - spacing;
                        else if (childBottom > visibleBottom)
                            scr.targetY = (float)(childBottom - rect.h) + spacing;

                        scr.targetY = std::clamp(scr.targetY, 0.0f, (float)maxScrollY);
                        // Focus-follow is intentional programmatic scroll, not user disengagement.
                        if (scr.targetY >= (float)maxScrollY - 2.0f) {
                            scr.stickToBottomY = true;
                        }
                        break;
                    }
                    scanY += ch + spacing;
                }
            }

            if (canScrollY && scr.targetY >= (float)maxScrollY - 2.0f) {
                scr.stickToBottomY = true;
            }

            scr.lastMaxScrollX = (float)maxScrollX;
            scr.lastMaxScrollY = (float)maxScrollY;

            

            SDL_Rect prevClip;
            bool hasClip = SDL_RenderIsClipEnabled(r);
            if (hasClip) SDL_RenderGetClipRect(r, &prevClip);
            SDL_Rect newClip = rect;
            if (hasClip) SDL_IntersectRect(&newClip, &prevClip, &newClip);
            SDL_RenderSetClipRect(r, &newClip);

            int currentX = rect.x - (int)scr.scrollX;
            int currentY = rect.y - (int)scr.scrollY;
            int stepGap  = spacing;

            if (flexSpaceY > 0 && totalFlexY == 0.0f) {
                if      (mainAlign == MainAxisAlignment::Center)                              currentY += flexSpaceY / 2;
                else if (mainAlign == MainAxisAlignment::End)                                 currentY += flexSpaceY;
                else if (mainAlign == MainAxisAlignment::SpaceBetween && childs.size() > 1)  stepGap  += flexSpaceY / (int)(childs.size() - 1);
                else if (mainAlign == MainAxisAlignment::SpaceEvenly) { stepGap += flexSpaceY / (int)(childs.size() + 1); currentY += stepGap - spacing; }
            }

            for (size_t i = 0; i < childs.size(); ++i) {
                const auto& c = childs[i];

                int ch = c.size.y;
                if (c.expandY > 0.0f)
                    ch = (totalFlexY > 0.0f) ? (int)(flexSpaceY * (c.expandY / totalFlexY)) : 0;

                int cw = rect.w;
                if (crossAlign != CrossAxisAlignment::Stretch && c.expandX <= 0.0f) {
                    cw = std::min((int)c.size.x, rect.w);
                }
                if (canScrollX) cw = std::max(cw, (int)c.size.x);

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

            const ScrollbarStyle& sb = st.scrollbarStyle;
            if (scroll != ScrollBehavior::None && !sb.hidden) {
                bool always = (scroll == ScrollBehavior::Always);
                bool showX  = canScrollX && maxScrollX > 0 && (always || isHovered || scr.scrollX > 0);
                bool showY  = canScrollY && maxScrollY > 0 && (always || isHovered || scr.scrollY > 0);

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
                    bool thumbHov = (in.mouseX >= thumbRect.x && in.mouseX <= thumbRect.x + thumbRect.w &&
                                     in.mouseY >= thumbRect.y && in.mouseY <= thumbRect.y + thumbRect.h);
                    FillRoundedBoxAA(r, trackRect, sb.radius, sb.trackColor);
                    FillRoundedBoxAA(r, thumbRect, sb.radius, thumbHov ? sb.thumbHoverColor : sb.thumbColor);
                }

                if (showY) {
                    int trackW = sb.thickness;
                    int trackX = rect.x + rect.w - trackW - 2;
                    SDL_Rect trackRect = {trackX, rect.y + 2, trackW, trackY_H};
                    float visibleRatio = actualTotalH > 0 ? std::clamp((float)rect.h / actualTotalH, 0.1f, 1.0f) : 1.0f;
                    int thumbH = std::max(20, (int)(trackRect.h * visibleRatio));
                    int thumbY = trackRect.y + (int)((scr.scrollY / maxScrollY) * (trackRect.h - thumbH));
                    SDL_Rect thumbRect = {trackX, thumbY, trackW, thumbH};
                    bool thumbHov = (in.mouseX >= thumbRect.x && in.mouseX <= thumbRect.x + thumbRect.w &&
                                     in.mouseY >= thumbRect.y && in.mouseY <= thumbRect.y + thumbRect.h);
                    FillRoundedBoxAA(r, trackRect, sb.radius, sb.trackColor);
                    FillRoundedBoxAA(r, thumbRect, sb.radius, thumbHov ? sb.thumbHoverColor : sb.thumbColor);
                }
            }

            // --- Column Debug Visualization ---
            // --- Column Debug Visualization ---
            if (dbg.enabled && dbg.showColumn) {
                    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

                    // Draw column bounds
                    SDL_SetRenderDrawColor(r, 255, 100, 100, 120);
                    SDL_RenderDrawRect(r, &rect);

                    // Draw vertical main axis line
                    SDL_SetRenderDrawColor(r, 255, 100, 100, 80);
                    SDL_RenderDrawLine(r, rect.x + rect.w / 2, rect.y,
                                        rect.x + rect.w / 2, rect.y + rect.h);

                    // ── Overflow detection ────────────────────────────────────────────
                    // Recompute the real content height (same logic as above, no scroll offset)
                    int contentH = 0;
                    for (const auto& c : childs) {
                        contentH += (c.expandY > 0.0f && totalFlexY > 0.0f)
                                        ? (int)(flexSpaceY * (c.expandY / totalFlexY))
                                        : (int)c.size.y;
                    }
                    if (!childs.empty()) contentH += spacing * (int)(childs.size() - 1);

                    int overflowY = contentH - rect.h;

                    if (overflowY > 0) {
                        // Red hatched zone below the column rect
                        SDL_Rect overflowRect = { rect.x, rect.y + rect.h, rect.w, overflowY };

                        // Solid tinted background
                        SDL_SetRenderDrawColor(r, 255, 50, 50, 55);
                        SDL_RenderFillRect(r, &overflowRect);

                        // Diagonal hatch lines (like DevTools)
                        SDL_SetRenderDrawColor(r, 255, 80, 80, 130);
                        const int HATCH_STEP = 8;
                        for (int offset = 0; offset < overflowRect.w + overflowRect.h; offset += HATCH_STEP) {
                            int x1 = overflowRect.x + offset;
                            int y1 = overflowRect.y;
                            int x2 = overflowRect.x;
                            int y2 = overflowRect.y + offset;
                            // Clamp to overflowRect bounds
                            if (x1 > overflowRect.x + overflowRect.w) {
                                y1 += x1 - (overflowRect.x + overflowRect.w);
                                x1  = overflowRect.x + overflowRect.w;
                            }
                            if (y2 > overflowRect.y + overflowRect.h) {
                                x2 += y2 - (overflowRect.y + overflowRect.h);
                                y2  = overflowRect.y + overflowRect.h;
                            }
                            SDL_RenderDrawLine(r, x1, y1, x2, y2);
                        }

                        // Dashed border around the overflow zone
                        SDL_SetRenderDrawColor(r, 255, 60, 60, 200);
                        const int DASH = 6;
                        for (int x = overflowRect.x; x < overflowRect.x + overflowRect.w; x += DASH * 2) {
                            SDL_RenderDrawLine(r, x,              overflowRect.y,
                                                x + DASH,       overflowRect.y);
                            SDL_RenderDrawLine(r, x,              overflowRect.y + overflowRect.h,
                                                x + DASH,       overflowRect.y + overflowRect.h);
                        }
                        SDL_RenderDrawLine(r, overflowRect.x,              overflowRect.y,
                                            overflowRect.x,              overflowRect.y + overflowRect.h);
                        SDL_RenderDrawLine(r, overflowRect.x + overflowRect.w, overflowRect.y,
                                            overflowRect.x + overflowRect.w, overflowRect.y + overflowRect.h);

                        // "overflow: Npx" label badge — rendered via SDL_ttf if you have a debug font,
                        // or just a filled pill as a placeholder if not
                        // Pill background
                        char label[32];
                        snprintf(label, sizeof(label), "overflow: %dpx", overflowY);
                        int labelX = rect.x + 4;
                        int labelY = rect.y + rect.h + 3;
                        SDL_Rect pill = { labelX - 2, labelY - 1, (int)(strlen(label) * 6) + 6, 13 };
                        SDL_SetRenderDrawColor(r, 220, 40, 40, 230);
                        SDL_RenderFillRect(r, &pill);
                        // If you have a small debug TTF font available, render `label` here.
                        // e.g.: DrawTextSmall(r, label, labelX, labelY, {255,255,255,255});
                    }
                }
            };
        
        return w;
    }
}