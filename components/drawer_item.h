#pragma once
#include <string>
#include "shared.h"

inline Widget DrawerItem(uint32_t iconCode, const std::string &label, const std::string &id, const WidgetNav &nav, AppFonts& fonts, TTF_Font* font = nullptr)
{
    using namespace Widgets;
    bool pywalEnabled = g_Context.pywalEnabled;
    WalTheme wal = g_Context.currentTheme;

        SDL_Color surface = GetThemeColor(
        DefaultTheme::SurfaceLight,
        wal.foreground,
        pywalEnabled
    );

    surface.a = 8;

    SDL_Color text = GetThemeColor(
        DefaultTheme::TextSecondary,
        wal.color7,
        pywalEnabled
    );

    SDL_Color hover = GetThemeColor(
        DefaultTheme::Hover,
        wal.color4,
        pywalEnabled
    );

    hover.a = 30;

    SDL_Color activeText = GetThemeColor(
        DefaultTheme::TextPrimary,
        wal.foreground,
        pywalEnabled
    );

    WidgetStyle style;
    style.color = surface;
    style.radius = 12;

    TTF_Font* usableFont = font ? font : fonts.fontAwesome24;

    Widget w =
        RoundedBox(
            {},
            style,
            Padding({20, 18},
                Row(
                    MainAxisAlignment::Start,
                    CrossAxisAlignment::Center,
                    20,
                    {
                        Icon({iconCode}, usableFont, 24, text),
                        Text(label, fonts.arial18, text)
                    }
                )
            ),
            {-1,0}
        );

    w.id = id;

    return Expanded(0,1,w
        .OnHover(id, 0.25f, [hover, activeText](Widget &w, float t) {

            SetCursor(CursorType::Hand);

            w.animateBorder(activeText, 2, t);

            w.children[0].children[0].children[0]
                .animateColor(activeText, t);

            w.children[0].children[0].children[1]
                .animateColor(activeText, t);
        })

        .OnFocus(id, 0.25f, [hover, activeText](Widget &w, float t) {

            w.animateBorder(activeText, 2, t);

            w.children[0].children[0].children[0]
                .animateColor(activeText, t);

            w.children[0].children[0].children[1]
                .animateColor(activeText, t);
        })

        .WithNav(nav.up, nav.down, nav.left, nav.right, nav.next, nav.prev)
    );
}