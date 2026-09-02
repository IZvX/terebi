#pragma once
#include "../shared.h"
#include "../nav_item_rounded.h"
#include "../settings_drawer.h"
#include "../drawer_toggle.h"
#include "../../terebi/remote_control.h"

inline Widget InputsInfoRow(const std::string& label, const std::string& value, AppFonts& fonts) {
    using namespace Widgets;
    return Row(MainAxisAlignment::SpaceBetween, CrossAxisAlignment::Center, 10, std::vector<Widget>{
        Text(label, fonts.arial18, {161, 161, 170, 255}, {}),
        Text(value.empty() ? "-" : value, fonts.arial18, {255, 255, 255, 255}, {})
    });
}

inline Widget SettingsTab_Inputs(AppFonts& fonts) {
    using namespace Widgets;
    bool pywalEnabled = g_Context.pywalEnabled;
    WalTheme wal = g_Context.currentTheme;
    TerebiRemote::Status remote = TerebiRemote::GetStatus();

    SDL_Color surface = GetThemeColor(DefaultTheme::SurfaceLight, wal.foreground, pywalEnabled);
    surface.a = 8;

    WidgetStyle cardStyle;
    cardStyle.color = surface;
    cardStyle.radius = 12;

    const std::string url = "ws://" + TerebiRemote::LocalAddress() + ":" + std::to_string(remote.port);
    const std::string state = remote.running
        ? (remote.paired ? "Connected" : "Waiting")
        : "Off";

    Widget infoCard = RoundedBox({0, 0}, cardStyle,
        Padding({20, 20}, Expanded(0, 1,
            Column(MainAxisAlignment::Start, CrossAxisAlignment::Stretch, 15, std::vector<Widget>{
                InputsInfoRow("Status", state, fonts),
                InputsInfoRow("Pair Code", remote.running ? remote.pairingCode : "-", fonts),
                InputsInfoRow("Remote URL", remote.running ? url : "-", fonts),
                InputsInfoRow("Client", remote.lastClient, fonts),
                InputsInfoRow("Error", remote.error, fonts)
            })
        ))
    );
    infoCard.id = "drawer_inputs_code";
    infoCard = infoCard.WithNav("drawer_inputs_osk", "drawer_inputs_last", "", "", "", "");

    Widget lastCard = RoundedBox({0, 0}, cardStyle,
        Padding({20, 20}, Expanded(0, 1,
            Column(MainAxisAlignment::Start, CrossAxisAlignment::Stretch, 10, std::vector<Widget>{
                Text("Last Remote Message", fonts.arial18, {255, 255, 255, 255}, {}),
                Text(remote.lastMessage.empty() ? "-" : remote.lastMessage, fonts.arial14, {161, 161, 170, 255}, {})
            })
        ))
    );
    lastCard.id = "drawer_inputs_last";
    lastCard = lastCard.WithNav("drawer_inputs_code", "drawer_inputs_back", "", "", "", "");

    return Column(MainAxisAlignment::Start, CrossAxisAlignment::Start, 20, std::vector<Widget>{
        Expanded(0, 1,
            Row(MainAxisAlignment::Start, CrossAxisAlignment::Center, 20, std::vector<Widget>{
                NavItemRounded(FontAwesome::ArrowLeft().codepoint, "Back", "drawer_inputs_back",
                    {"drawer_inputs_last", "drawer_inputs_enable", "", "", "", ""}, fonts)
                .OnClick("drawer_inputs_back", 0.25f, []() {
                    SetDrawerTab("settings_drawer", "main");
                    g_NextFocusedWidgetId = "drawer_settings_main_inputs";
                }, [](Widget&, float){}),
                Text("Inputs", fonts.arial28, {255, 255, 255, 255}, {})
            })
        ),

        Expanded(0, 1,
            Column(MainAxisAlignment::Start, CrossAxisAlignment::Stretch, 10, std::vector<Widget>{
                DrawerToggle(g_Context.remoteEnabled, 0xf11c, "Web Remote",
                    "drawer_inputs_enable",
                    {"drawer_inputs_back", "drawer_inputs_osk", "", "", "", ""},
                    fonts,
                    [](bool enabled) {
                        if (enabled) TerebiRemote::Start(3000);
                        else TerebiRemote::Stop();
                    }),

                DrawerToggle(g_Context.oskEnabled, 0xf031, "On-Screen Keyboard",
                    "drawer_inputs_osk",
                    {"drawer_inputs_enable", "drawer_inputs_code", "", "", "", ""},
                    fonts,
                    [](bool enabled) {
                        if (!enabled) g_Context.oskVisible = false;
                    }),

                infoCard,
                lastCard
            })
        )
    });
}
