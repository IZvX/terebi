#pragma once
#include <string>
#include "shared.h"

inline Widget NavItem(uint32_t iconCode, const std::string &label, const std::string &id, const WidgetNav &nav, AppFonts& fonts)
{
    using namespace Widgets;
    WidgetStyle style;
    style.color = {0, 0, 0, 0};
    style.radius = 12;
    
    Widget w = RoundedBox(
        {}, style, {Padding({16, 8}, Text(label, fonts.arial18, {161, 161, 170, 255}))});
    w.id = id;

    return w
        .OnHover(id, 0.25f,[](Widget &w, float t) {
            SetCursor(CursorType::Hand);
            w.animateColor({255, 255, 255, 25}, t);
            w.children[0].children[0].animateColor({255, 255, 255, 255}, t);
        })
        .OnFocus(id, 0.25f,[](Widget &w, float t) {
            w.animateColor({255, 255, 255, 25}, t);
            w.children[0].children[0].animateColor({255, 255, 255, 255}, t);
        })
        .WithNav(nav.up, nav.down, nav.left, nav.right, nav.next, nav.prev);
}