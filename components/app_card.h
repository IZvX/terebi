#pragma once

#include <string>
#include "../nimble/application.h"
#include "../components/shared.h"
#include "../utils/theme.h"

inline Widget AppCard(
    const std::string& id,
    const std::string& title,
    IconData icon,
    TTF_Font* iconFont,
    const std::string& command,
    WidgetNav nav,
    AppFonts& fonts,
    SDL_Color accentColor = { 45, 125, 245, 255 })
{
    using namespace Widgets;

    // 1. Base card styling
    WidgetStyle cardStyle;
    cardStyle.radius = 16;
    cardStyle.borderWidth = 1;
    cardStyle.borderColor = { 255, 255, 255, 35 };
    cardStyle.color = { 20, 22, 28, 140 }; // Semi-transparent dark glass

    SDL_Color iconBgColor = accentColor;
    iconBgColor.a = 35;

    Widget iconBadge = RoundedBox({ 52, 52 }, 
        WidgetStyle{
            .color = iconBgColor,
            .radius = 12,
            .borderWidth = 1,
            .borderColor = accentColor
        },
        {
            Center(Icon(icon, iconFont, 24, accentColor))
        }
    );

    Widget cardContent = Column(MainAxisAlignment::Center, CrossAxisAlignment::Center, 12, {
        iconBadge,
        Text(title, fonts.spaceGrotesk12 ? fonts.spaceGrotesk12 : fonts.arial14, { 240, 240, 245, 255 }, {})
    });

    Widget card = RoundedBox({ 160, 130 }, cardStyle, {
        Center(cardContent)
    });

    // 2. Set ID, Navigation, and Enter-key activation
    card.id = id;
    card.nav = nav;
    card.captureFocusOnClick = true;
    card.enterTriggersClick = true;

    // 3. Click / Enter key action
    card.onClickFn = [command]() {
        // server_launch_app(command.c_str());
    };

    // 4. Hover & Focus animations
    card.OnHover(id + "_hover", 0.18f, [accentColor](Widget &w, float t) {
        w.style.color = { (Uint8)(20 + 20 * t), (Uint8)(22 + 25 * t), (Uint8)(28 + 35 * t), (Uint8)(140 + 70 * t) };
        w.style.borderColor = LerpColor({ 255, 255, 255, 35 }, accentColor, t);
        w.style.borderWidth = (int)(1 + 1 * t);
    });

    card.OnFocus(id + "_focus", 0.2f, [accentColor](Widget &w, float t) {
        w.style.borderColor = { 255, 255, 255, (Uint8)(200 + 55 * t) };
        w.style.borderWidth = 2;
        w.style.color = { 35, 42, 56, 220 };
    });

    return card;
}