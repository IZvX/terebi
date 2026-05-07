// components/settings_drawer.h
#pragma once
#include "shared.h"

// Include your individual tabs
#include "settings_drawer/main_tab.h"
#include "settings_drawer/network_tab.h"
#include "settings_drawer/interface_tab.h"
#include "settings_drawer/system_tab.h"

inline Widget RenderSettingsDrawer(int ww, int wh, float dt, AppFonts& fonts) {
    return SideDrawer("settings_drawer", g_Context.settingsOpen, ww, wh, dt, {
        {"main",                  [&]() -> Widget { return SettingsTab_Main(fonts); }},
        {"network",               [&]() -> Widget { return SettingsTab_Network(fonts); }},
        {"network_details",       [&]() -> Widget { return SettingsTab_NetworkDetails(fonts); }},
        {"interface",             [&]() -> Widget { return SettingsTab_Interface(fonts); }},
        {"system",             [&]() -> Widget { return SettingsTab_System(fonts); }},
    });
}