#pragma once
#include <string>
#include "shared.h"

inline Widget NavItem(
    uint32_t iconCode,
    const std::string &label,
    const std::string &id,
    const WidgetNav &nav,
    AppFonts& fonts
)
{
    using namespace Widgets;
    bool pywalEnabled = g_Context.pywalEnabled;
    WalTheme wal = g_Context.currentTheme;


    SDL_Color hover = GetThemeColor(
        DefaultTheme::Hover,
        wal.background,
        pywalEnabled
    );

    hover.a = 75;

    SDL_Color textColor = GetThemeColor(
        DefaultTheme::TextSecondary,
        wal.color7,
        pywalEnabled
    );

    SDL_Color activeText = GetThemeColor(
        DefaultTheme::TextPrimary,
        wal.foreground,
        pywalEnabled
    );

    SDL_Color borderColor = GetThemeColor(
        DefaultTheme::TextPrimary,
        wal.foreground,
        pywalEnabled
    );


    WidgetStyle style;
    style.color = {0,0,0,0};
    style.radius = 12;

    Widget w = RoundedBox(
        {},
        style,
        {
            Padding(
                {16, 8},
                Text(label, fonts.arial18, textColor)
            )
        }
    );

    w.id = id;

    return w

        .OnHover(id, 0.25f,[borderColor,hover, activeText](Widget &w, float t) {

            SetCursor(CursorType::Hand);

            w.animateBorder(borderColor, 2, t);
            w.animateColor(hover, t);

            w.children[0].children[0]
                .animateColor(activeText, t);
        })

        .OnFocus(id, 0.25f,[borderColor,hover, activeText](Widget &w, float t) {

            w.animateBorder(borderColor, 2, t);
            w.animateColor(hover, t);

            w.children[0].children[0]
                .animateColor(activeText, t);
        })

        .WithNav(nav.up, nav.down, nav.left, nav.right, nav.next, nav.prev);
}