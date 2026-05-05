#pragma once
#include <string>
#include "shared.h"

inline Widget DrawerItem(uint32_t iconCode, const std::string &label, const std::string &id, const WidgetNav &nav, AppFonts& fonts)
{
    using namespace Widgets;

    WidgetStyle style;
    style.color = {255, 255, 255, 8};
    style.radius = 12;

    Widget w =
        RoundedBox(
            {},
            style,
            Expanded(1,0,Padding({20, 18},
                    Row(
                        MainAxisAlignment::Start,
                        CrossAxisAlignment::Center,
                        20,
                        {
                            Icon({iconCode}, fonts.fontAwesome24, 24, {161,161,170,255}),
                            Text(label, fonts.arial18, {161,161,170,255})
                        }
                    )
                )
            ),
            {-1,0}
        );

    w.id = id;

    return Expanded(0,1,w
        .OnHover(id, 0.25f, [](Widget &w, float t) {
            SetCursor(CursorType::Hand);
            w.animateColor({255, 255, 255, 25}, t);
            w.animateBorder({255,255,255,255},2,t);
            w.children[0].children[0].children[0].animateColor({255, 255, 255, 255}, t);
            w.children[0].children[0].children[1].animateColor({255, 255, 255, 255}, t);
        })
        .OnFocus(id, 0.25f, [](Widget &w, float t) {
            w.animateColor({255, 255, 255, 25}, t);
            w.animateBorder({255,255,255,255},2,t);
            w.children[0].children[0].children[0].animateColor({255, 255, 255, 255}, t);
            w.children[0].children[0].children[1].animateColor({255, 255, 255, 255}, t);
        })
        .WithNav(nav.up, nav.down, nav.left, nav.right, nav.next, nav.prev)
    );
}