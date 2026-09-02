#pragma once
#include "../shared.h"
#include "../drawer.h"
#include "../drawer_item.h"
#include "../../fontawesome/fontawesome.h"
// #include "../../compositor/wlroots.h"

inline Widget SettingsTab_Main(AppFonts& fonts) {
    using namespace Widgets;
    
    float disabledOpacity = 0.5f;

    return Column(MainAxisAlignment::Start, CrossAxisAlignment::Start, 20, {
        // Header Row
        Expanded(0,1,
            Row(MainAxisAlignment::SpaceBetween, CrossAxisAlignment::Center, 0, {
                Text("Settings", fonts.arial28, {255,255,255,255}, {}),
                NavItemRounded(FontAwesome::Times().codepoint, "Close Settings", "drawer_settings_closesettings",
                { "drawer_settings_main_system", "drawer_settings_main_network", "", "", "", "" }, fonts)
                .OnClick("navbar_settings", 0.25f, []() {
                    g_Context.settingsOpen = false;
                    // if (g_terebi_shared) {
                        // g_terebi_shared->drawer_open = false;
                    // }
                    g_NextFocusedWidgetId = "navbar_settings";
                }, [](Widget &w, float t) {})
            })
        ),

        // Settings List
        Expanded(0,1,
            Column(MainAxisAlignment::Start, CrossAxisAlignment::Start, 10, {
                
                // 1. Network (Enabled)
                DrawerItem(FontAwesome::Wifi().codepoint, "Network & Internet", "drawer_settings_main_network",
                { "drawer_settings_closesettings", "drawer_settings_main_display", "", "", "", "" }, fonts)
                .OnClick("drawer_settings_main_network", 0.2f, []() { 
                    SetDrawerTab("settings_drawer", "network"); g_NextFocusedWidgetId = "drawer_network_back"; 
                }, [](Widget&, float){}),

                // 2. Display (Disabled)
                Opacity(disabledOpacity, 
                    DrawerItem(FontAwesome::Desktop().codepoint, "Display", "drawer_settings_main_display",
                    { "drawer_settings_main_network", "drawer_settings_main_sound", "", "", "", "" }, fonts)
                    .WithDisabled(true)
                ),

                // 3. Sound (Disabled)
                Opacity(disabledOpacity,
                    DrawerItem(FontAwesome::VolumeUp().codepoint, "Sound", "drawer_settings_main_sound",
                    { "drawer_settings_main_display", "drawer_settings_main_apps", "", "", "", "" }, fonts)
                    .WithDisabled(true)
                ),

                // 4. Apps (Disabled)
                Opacity(disabledOpacity,
                    DrawerItem(FontAwesome::Grid().codepoint, "Apps", "drawer_settings_main_apps",
                    { "drawer_settings_main_sound", "drawer_settings_main_interface", "", "", "", "" }, fonts)
                    .WithDisabled(true)
                ),

                // 5. Interface (Enabled)
                DrawerItem(FontAwesome::PaintBrush().codepoint, "Interface", "drawer_settings_main_interface",
                { "drawer_settings_main_apps", "drawer_settings_main_inputs", "", "", "", "" }, fonts)
                .OnClick("drawer_settings_main_interface", 0.2f, []() { 
                    SetDrawerTab("settings_drawer", "interface"); g_NextFocusedWidgetId = "drawer_interface_back"; 
                }, [](Widget&, float){}),

                // 6. Inputs (Enabled)
                DrawerItem(0xf11c, "Inputs", "drawer_settings_main_inputs",
                { "drawer_settings_main_interface", "drawer_settings_main_accessibility", "", "", "", "" }, fonts)
                .OnClick("drawer_settings_main_inputs", 0.2f, []() {
                    SetDrawerTab("settings_drawer", "inputs"); g_NextFocusedWidgetId = "drawer_inputs_back";
                }, [](Widget&, float){}),

                // 7. Accessibility (Disabled)
                Opacity(disabledOpacity,
                    DrawerItem(FontAwesome::UniversalAccess().codepoint, "Accessibility", "drawer_settings_main_accessibility",
                    { "drawer_settings_main_inputs", "drawer_settings_main_system", "", "", "", "" }, fonts)
                    .WithDisabled(true)
                ),

                // 8. System (Enabled)
                DrawerItem(FontAwesome::Cogs().codepoint, "System", "drawer_settings_main_system",
                { "drawer_settings_main_accessibility", "drawer_settings_closesettings", "", "", "", "" }, fonts)
                .OnClick("drawer_settings_main_system", 0.2f, []() { 
                    SetDrawerTab("settings_drawer", "system"); g_NextFocusedWidgetId = "drawer_system_back"; 
                }, [](Widget&, float){})
            })
        )
    });
}
