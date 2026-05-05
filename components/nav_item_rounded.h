#pragma once
#include <string>
#include "shared.h"

inline Widget NavItemRounded(uint32_t iconCode, const std::string &label, const std::string &id, const WidgetNav &nav, AppFonts& fonts)
{
    using namespace Widgets;
    SDL_Color bgColor = {255, 255, 255, 13};
    SDL_Color iconColor = {255, 255, 255, 255};

    WidgetStyle style;
    style.color = bgColor;
    style.radius = 22;

    Widget w = RoundedBox({44, 44}, style, {Icon({iconCode}, fonts.fontAwesome24, 18, iconColor, {}, 0)});
    w.id = id;

    return w
        .OnHover(id, 0.25f,[](Widget &w, float t) {
            SetCursor(CursorType::Hand);
            w.animateBorder({255, 255, 255, 255}, 3, t);
        })
        .OnFocus(id, 0.25f,[](Widget &w, float t) { 
            w.animateBorder({255, 255, 255, 255}, 3, t); 
        })
        .WithNav(nav.up, nav.down, nav.left, nav.right, nav.next, nav.prev);
}