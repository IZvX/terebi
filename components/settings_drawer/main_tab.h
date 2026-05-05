#pragma once
#include "../shared.h"
#include "../drawer.h"
#include "../drawer_item.h"
#include "../../fontawesome/fontawesome.h"

inline Widget SettingsTab_Main(AppFonts& fonts) {
    using namespace Widgets;
    return Column(MainAxisAlignment::Start, CrossAxisAlignment::Start, 20, {
        Expanded(0,1,
            Row(MainAxisAlignment::SpaceBetween, CrossAxisAlignment::Center, 0, {
                Text("Settings",fonts.arial28,{255,255,255,255},{}),
                NavItemRounded(FontAwesome::Times().codepoint,"Close Settings","drawer_settings_closesettings",
                { "drawer_settings_main_system", "drawer_settings_main_network", "", "", "", "" },fonts)
                .OnClick("navbar_settings", 0.25f,[]() {
                    g_Context.settingsOpen = !g_Context.settingsOpen; 
                    g_NextFocusedWidgetId = "navbar_settings";
                },[](Widget &w, float t) {})
            })
        ),
        Expanded(0,1,
            Column(MainAxisAlignment::Start, CrossAxisAlignment::Start,10, {
                DrawerItem(FontAwesome::Wifi().codepoint,"Network & Internet","drawer_settings_main_network",
                { "drawer_settings_closesettings", "drawer_settings_main_display", "", "", "", "" },fonts)
                .OnClick("drawer_settings_main_network", 0.2f,[]() { 
                    SetDrawerTab("settings_drawer", "network"); g_NextFocusedWidgetId = "drawer_network_back"; 
                },[](Widget&, float){}),

                DrawerItem(FontAwesome::Desktop().codepoint,"Display","drawer_settings_main_display",
                { "drawer_settings_main_network", "drawer_settings_main_sound", "", "", "", "" },fonts)
                .OnClick("drawer_settings_main_display", 0.2f,[]() { 
                    SetDrawerTab("settings_drawer", "display"); g_NextFocusedWidgetId = "drawer_display_back"; 
                },[](Widget&, float){}),

                DrawerItem(FontAwesome::VolumeUp().codepoint,"Sound","drawer_settings_main_sound",
                { "drawer_settings_main_display", "drawer_settings_main_apps", "", "", "", "" },fonts)
                .OnClick("drawer_settings_main_sound", 0.2f,[]() { 
                    SetDrawerTab("settings_drawer", "sound"); g_NextFocusedWidgetId = "drawer_sound_back"; 
                },[](Widget&, float){}),

                DrawerItem(FontAwesome::Grid().codepoint,"Apps","drawer_settings_main_apps",
                { "drawer_settings_main_sound", "drawer_settings_main_interface", "", "", "", "" },fonts)
                .OnClick("drawer_settings_main_apps", 0.2f,[]() { 
                    SetDrawerTab("settings_drawer", "apps"); g_NextFocusedWidgetId = "drawer_apps_back"; 
                },[](Widget&, float){}),

                DrawerItem(FontAwesome::PaintBrush().codepoint,"Interface","drawer_settings_main_interface",
                { "drawer_settings_main_apps", "drawer_settings_main_accessibility", "", "", "", "" },fonts)
                .OnClick("drawer_settings_main_interface", 0.2f,[]() { 
                    SetDrawerTab("settings_drawer", "interface"); g_NextFocusedWidgetId = "drawer_interface_back"; 
                },[](Widget&, float){}),

                DrawerItem(FontAwesome::UniversalAccess().codepoint,"Accessibility","drawer_settings_main_accessibility",
                { "drawer_settings_main_interface", "drawer_settings_main_system", "", "", "", "" },fonts)
                .OnClick("drawer_settings_main_accessibility", 0.2f,[]() { 
                    SetDrawerTab("settings_drawer", "accessibility"); g_NextFocusedWidgetId = "drawer_accessibility_back"; 
                },[](Widget&, float){}),

                DrawerItem(FontAwesome::Cogs().codepoint,"System","drawer_settings_main_system",
                { "drawer_settings_main_accessibility", "drawer_settings_closesettings", "", "", "", "" },fonts)
                .OnClick("drawer_settings_main_system", 0.2f,[]() { 
                    SetDrawerTab("settings_drawer", "system"); g_NextFocusedWidgetId = "drawer_system_back"; 
                },[](Widget&, float){})
            })
        )
    });
}