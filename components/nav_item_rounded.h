#pragma once
#include <string>
#include "shared.h"

inline Widget NavItemRounded(uint32_t iconCode, const std::string &label, const std::string &id, const WidgetNav &nav, AppFonts& fonts, TTF_Font* font = nullptr)
{
    using namespace Widgets;
    bool pywalEnabled = g_Context.pywalEnabled;
    WalTheme wal = g_Context.currentTheme;

    SDL_Color bgColor = GetThemeColor(
        DefaultTheme::SurfaceLight,
        wal.foreground,
        pywalEnabled
    );

    bgColor.a = 13;

    SDL_Color iconColor = GetThemeColor(
        DefaultTheme::TextPrimary,
        wal.foreground,
        pywalEnabled
    );

    SDL_Color borderColor = GetThemeColor(
        DefaultTheme::Border,
        wal.foreground,
        pywalEnabled
    );

    WidgetStyle style;
    style.color = bgColor;
    style.radius = 22;

    TTF_Font* usableFont = font ? font : fonts.fontAwesome24;
    Widget w = RoundedBox(
        {44, 44},
        style,
        {
            Icon({iconCode}, usableFont, 18, iconColor, {}, 0)
        }
    );

    w.id = id;

    return w
        .OnHover(id, 0.25f,[borderColor](Widget &w, float t) {

            SetCursor(CursorType::Hand);

            w.animateBorder(borderColor, 3, t);
        })

        .OnFocus(id, 0.25f,[borderColor](Widget &w, float t) {

            w.animateBorder(borderColor, 3, t);
        })

        .WithNav(nav.up, nav.down, nav.left, nav.right, nav.next, nav.prev);
}