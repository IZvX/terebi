#pragma once
#include "../shared.h"
#include "../nav_item_rounded.h"
#include "../settings_drawer.h"
#include "../drawer_toggle.h"
#include "../drawer.h"

inline Widget SettingsTab_Interface(AppFonts& fonts) {
    using namespace Widgets;

    // Reload button styling
    WidgetStyle buttonStyle;
    buttonStyle.color = {255, 255, 255, 8};
    buttonStyle.radius = 12;

    // Reload button
    Widget reloadButton = RoundedBox(
        {},
        buttonStyle,
        Expanded(1, 0, Padding({20, 18},
            Row(
                MainAxisAlignment::Start,
                CrossAxisAlignment::Center,
                20,
                {
                    Icon({0xf021}, fonts.fontAwesome24, 24, {161, 161, 170, 255}), // reload icon
                    Text("Reload Theme", fonts.arial18, {161, 161, 170, 255})
                }
            )
        )),
        {-1, 0}
    );
    reloadButton.id = "drawer_interface_reload_theme";

    reloadButton.OnClick("drawer_interface_reload_theme_click", 0, []() {
        ReloadPywalTheme();
    }, [](Widget&, float){});

    // Safe hover handler that wraps child access in try/catch
    reloadButton.OnHover("drawer_interface_reload_theme", 0.25f, [](Widget &w, float t) {
        SetCursor(CursorType::Hand);
        w.animateColor({255, 255, 255, 25}, t);
        w.animateBorder({255, 255, 255, 255}, 2, t);
        try {
            // Structure: RoundedBox -> Expanded -> Padding -> Row -> {Icon, Text}
            auto& contentRow = w.children[0].children[0].children[0];
            if (contentRow.children.size() >= 2) {
                contentRow.children[0].animateColor({255, 255, 255, 255}, t); // Icon
                contentRow.children[1].animateColor({255, 255, 255, 255}, t); // Text
            }
        } catch (...) {}
    });

    reloadButton.OnFocus("drawer_interface_reload_theme", 0.25f, [](Widget &w, float t) {
        w.animateColor({255, 255, 255, 25}, t);
        w.animateBorder({255, 255, 255, 255}, 2, t);
        try {
            // Structure: RoundedBox -> Expanded -> Padding -> Row -> {Icon, Text}
            auto& contentRow = w.children[0].children[0].children[0];
            if (contentRow.children.size() >= 2) {
                contentRow.children[0].animateColor({255, 255, 255, 255}, t); // Icon
                contentRow.children[1].animateColor({255, 255, 255, 255}, t); // Text
            }
        } catch (...) {}
    });

    reloadButton = Expanded(0, 1, reloadButton
        .WithNav("drawer_interface_pywal_toggle", "drawer_interface_back", "", "", "", "")
    );

    return Column(MainAxisAlignment::Start, CrossAxisAlignment::Start, 20, {
        Expanded(0, 1,
            Row(MainAxisAlignment::Start, CrossAxisAlignment::Center, 20, {
                NavItemRounded(FontAwesome::ArrowLeft().codepoint, "Back", "drawer_interface_back",
                    {"drawer_interface_reload_theme", "drawer_interface_pywal_toggle", "", "", "", ""},
                    fonts)
                .OnClick("drawer_settings_main_interface", 0.25f, []() {
                    SetDrawerTab("settings_drawer", "main");
                    g_NextFocusedWidgetId = "drawer_settings_main_interface";
                }, [](Widget &w, float t) {}),
                Text("Interface", fonts.arial28, {255, 255, 255, 255}, {})
            })
        ),
        Expanded(0, 1,
            Column(MainAxisAlignment::Start, CrossAxisAlignment::Start, 10, {
                DrawerToggle(g_Context.pywalEnabled, 0xf042, "Use Pywal Theme",
                    "drawer_interface_pywal_toggle",
                    {"drawer_interface_back", "drawer_interface_reload_theme", "", "", "", ""},
                    fonts,
                    [](bool enabled) {
                        if (!enabled) {
                            // If disabling, we just set the flag - the app will use default colors
                        } else {
                            // If enabling, load the pywal theme
                            ReloadPywalTheme();
                        }
                    }
                ),
                reloadButton
            })
        )
    });
}
