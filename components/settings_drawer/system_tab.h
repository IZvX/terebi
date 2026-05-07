#pragma once
#include "../shared.h"
#include "../nav_item_rounded.h"
#include "../settings_drawer.h"
#include "../drawer_toggle.h"
#include "../drawer_item.h"

inline Widget SystemInfoRow(const std::string& label, const std::string& value, AppFonts& fonts) {
    using namespace Widgets;
    return Row(MainAxisAlignment::SpaceBetween, CrossAxisAlignment::Center, 10, std::vector<Widget>{
        Text(label, fonts.arial18, {161, 161, 170, 255}, {}),
        Text(value, fonts.arial18, {255, 255, 255, 255}, {})
    });
}

inline Widget SettingsTab_System(AppFonts& fonts) {
    using namespace Widgets;
    bool pywalEnabled = g_Context.pywalEnabled;
    WalTheme wal = g_Context.currentTheme;

    SDL_Color surface = GetThemeColor(DefaultTheme::SurfaceLight, wal.foreground, pywalEnabled);
    surface.a = 8;

    WidgetStyle cardStyle;
    cardStyle.color  = surface;
    cardStyle.radius = 12;

    // --- Main Layout ---
    return Column(MainAxisAlignment::Start, CrossAxisAlignment::Start, 20, std::vector<Widget>{
        // Header
        Expanded(0, 1,
            Row(MainAxisAlignment::Start, CrossAxisAlignment::Center, 20, std::vector<Widget>{
                NavItemRounded(FontAwesome::ArrowLeft().codepoint, "Back", "drawer_system_back",
                    {"drawer_system_instagram", "drawer_system_dev", "", "", "", ""}, fonts)
                .OnClick("drawer_system_back", 0.25f, []() {
                    SetDrawerTab("settings_drawer", "main");
                    g_NextFocusedWidgetId = "drawer_settings_main_system";
                }, [](Widget&, float){}),
                Text("System", fonts.arial28, {255, 255, 255, 255}, {})
            })
        ),
        
        // Content
        Expanded(1, 1,
            Column(MainAxisAlignment::Start, CrossAxisAlignment::Stretch, 20, std::vector<Widget>{
                // Branding
                Padding({48, 20}, Column(MainAxisAlignment::Center, CrossAxisAlignment::Center, 5, std::vector<Widget>{
                    Text("terebi", fonts.spaceGrotesk48, {255, 255, 255, 255}),
                    Text("Build v1.0.42-alpha | powered by nimble.", fonts.spaceGrotesk12, {255, 255, 255, 100})
                }),1,0),

                Text("Device Specifications", fonts.arial18, {255, 255, 255, 255}),
                RoundedBox({0, 0}, cardStyle,
                    Padding({20, 20}, Expanded(0,1,Column(MainAxisAlignment::Start, CrossAxisAlignment::Stretch, 15, std::vector<Widget>{
                        SystemInfoRow("OS", "arch linux 1.0", fonts),
                        SystemInfoRow("Kernel", "6.6.20-v8+", fonts),
                        SystemInfoRow("Processor", "Broadcom BCM2712", fonts),
                        SystemInfoRow("Memory", "3.2 GB / 8.0 GB", fonts)
                    })))
                ),

                Text("Advanced", fonts.arial18, {255, 255, 255, 255}),
                DrawerItem(FontAwesome::Cogs().codepoint, "Developer Settings", "drawer_system_dev", 
                    {"drawer_system_back", "drawer_system_pi", "", "", "", ""}, fonts),
                DrawerItem(FontAwesome::RaspberryPi().codepoint, "Pi Configuration", "drawer_system_pi",
                    {"drawer_system_dev", "drawer_system_instagram", "", "", "", ""}, fonts,fonts.fontAwesomeB24),

                // Social Row
                Expanded(1,1,Row(MainAxisAlignment::SpaceBetween, CrossAxisAlignment::End, 10, std::vector<Widget>{
                    NavItemRounded(FontAwesome::Instagram().codepoint, "Instagram", "drawer_system_instagram", 
                        {"drawer_system_pi", "drawer_system_back", "", "drawer_system_github", "", ""}, fonts, fonts.fontAwesomeB24),
                    NavItemRounded(FontAwesome::Github().codepoint, "Github", "drawer_system_github", 
                        {"drawer_system_pi", "drawer_system_back", "drawer_system_instagram", "drawer_system_discord", "", ""}, fonts, fonts.fontAwesomeB24),
                    NavItemRounded(FontAwesome::Discord().codepoint, "Discord", "drawer_system_discord", 
                        {"drawer_system_pi", "drawer_system_back", "drawer_system_github", "", "", ""}, fonts, fonts.fontAwesomeB24)
                }))
            })
        )
    });
}