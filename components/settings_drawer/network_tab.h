#pragma once
#include "../shared.h"
#include "../nav_item_rounded.h"
#include "../settings_drawer.h"

inline Widget SettingsTab_Network(AppFonts& fonts) {
    using namespace Widgets;
    return Column(MainAxisAlignment::Start, CrossAxisAlignment::Start, 20, {
        Expanded(0,1, Row(MainAxisAlignment::Start, CrossAxisAlignment::Center, 15, {
            NavItemRounded(FontAwesome::ArrowLeft().codepoint,"Back","drawer_network_back",
            { "", "", "", "", "", "" },fonts)
            .OnClick("drawer_network_back", 0.25f,[]() { 
                SetDrawerTab("settings_drawer", "main"); g_NextFocusedWidgetId = "drawer_settings_main_network"; 
            },[](Widget &w, float t) {}),
            Text("Network & Internet",fonts.arial28,{255,255,255,255},{}),
        })),
        Expanded(0,1, Column(MainAxisAlignment::Start, CrossAxisAlignment::Start,10, {}))
    });
}