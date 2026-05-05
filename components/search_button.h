#pragma once
#include <string>
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include "shared.h"

struct SearchButtonState {
    bool expanded = false;  
    float t = 0.0f;         
    bool focusSent = false; 
};

// Global state map, preserved across frames
inline std::unordered_map<std::string, SearchButtonState> g_SearchState;

inline Widget SearchButton(uint32_t iconCode, const std::string &id, const WidgetNav &nav, float dt, std::string &search, AppFonts& fonts)
{
    using namespace Widgets;
    SearchButtonState &ss = g_SearchState[id];

    const float expandSpeed = 1.0f / 0.35f;
    const float collapseSpeed = 1.0f / 0.25f;

    bool isActive = (g_FocusedWidgetId == id || g_FocusedWidgetId == "nav_searchbar_text" ||
                     g_NextFocusedWidgetId == id ||                  
                     g_NextFocusedWidgetId == "nav_searchbar_text"); 

    if (ss.expanded && !isActive) {
        ss.expanded = false;
        ss.focusSent = false;
    }

    if (ss.expanded)
        ss.t = std::min(ss.t + dt * expandSpeed, 1.0f);
    else
        ss.t = std::max(ss.t - dt * collapseSpeed, 0.0f);

    float ease = 1.0f - (1.0f - ss.t) * (1.0f - ss.t) * (1.0f - ss.t);

    const int kCollapsed = 44;
    const int kExpanded = 250;
    const int kTextW = kExpanded - kCollapsed;

    int totalW = static_cast<int>(kCollapsed + (kExpanded - kCollapsed) * ease);
    int textW = static_cast<int>(kTextW * ease);

    SDL_Color bgColor = {255, 255, 255, 13};
    SDL_Color borderColor = {244, 244, 245, 255};
    SDL_Color iconColor = {255, 255, 255, 255};

    if (ss.t >= 0.99f && !ss.focusSent) {
        g_NextFocusedWidgetId = "nav_searchbar_text";
        ss.focusSent = true;
    }

    TextFieldStyle tfStyle;
    tfStyle.font = fonts.arial18;
    tfStyle.backgroundColor = {255, 255, 255, 0};
    tfStyle.borderColor = {0, 0, 0, 0};
    tfStyle.focusedBorderColor = {0, 0, 0, 0};
    tfStyle.textColor = {255, 255, 255, 255};
    tfStyle.placeholderColor = {120, 120, 120, 255};
    tfStyle.cursorColor = {255, 255, 255, 255};
    tfStyle.radius = 0;
    tfStyle.padding = {22, 4};
    tfStyle.borderWidth = 3;

    WidgetStyle outerStyle;
    outerStyle.color = bgColor;
    outerStyle.radius = 22;
    outerStyle.borderColor = borderColor;

    Widget tf = TextField("nav_searchbar_text", search, "Search…", {textW, kCollapsed}, tfStyle)
        .WithNav("", "", "navbar_library", "navbar_settings", "navbar_settings", "navbar_library")
        .OnFocus(id, 0.25f,[](Widget &w, float t) { w.animateBorder({255, 0, 255, 255}, 3, t); })
        .OnSubmit([&](std::string val) {
            search = val;
            std::cout << "Search submitted: " << search << std::endl;
        });

    Widget w = RoundedBox({totalW, kCollapsed}, outerStyle, {
        Row(MainAxisAlignment::End, CrossAxisAlignment::Center, 0, {
            Clip("sb_clip", {textW, kCollapsed}, 0, tf),
            Box({kCollapsed, kCollapsed}, {}, {Icon({iconCode}, fonts.fontAwesome24, 18, iconColor, {}, 0)}),
        })
    });

    w.id = id;
    w.OnHover(id, 0.25f,[](Widget &w, float t) {
        SetCursor(CursorType::Hand);
        w.animateBorder({255, 255, 255, 255}, 3, t);
    });
    w.OnFocus(id, 0.25f,[](Widget &w, float t) { 
        w.animateBorder({255, 255, 255, 255}, 3, t); 
    });

    return w.WithNav(nav.up, nav.down, nav.left, nav.right, nav.next, nav.prev);
}