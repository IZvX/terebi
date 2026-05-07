#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include "shared.h"

struct DrawerState {
    float t       = 0.0f;
    std::string activeTab = "main";
    std::string prevTab   = "";
    float tabT    = 1.0f;
};

inline std::unordered_map<std::string, DrawerState> g_DrawerStates;

inline void SetDrawerTab(const std::string& drawerId, const std::string& tabId) {
    DrawerState& state = g_DrawerStates[drawerId];
    if (tabId != state.activeTab) {
        state.prevTab   = state.activeTab;
        state.activeTab = tabId;
        state.tabT      = 0.0f;
    }
}

inline Widget SideDrawer(
    const std::string& id,
    bool isOpen,
    int ww, int wh,
    float dt,
    const std::unordered_map<std::string, std::function<Widget()>>& tabs)
{
    using namespace Widgets;
        bool pywalEnabled = g_Context.pywalEnabled;
    WalTheme wal = g_Context.currentTheme;


    SDL_Color drawerBg = GetThemeColor(
        DefaultTheme::BackgroundSecondary,
        wal.background,
        pywalEnabled
    );

    drawerBg.a = 217;
    DrawerState& state = g_DrawerStates[id];

    if (state.activeTab.empty() || tabs.find(state.activeTab) == tabs.end())
        state.activeTab = "main";

    // Drawer open/close animation
    const float drawerSpeed = 3.5f;
    if (isOpen) state.t = std::min(state.t + dt * drawerSpeed, 1.0f);
    else        state.t = std::max(state.t - dt * drawerSpeed, 0.0f);

    if (state.t <= 0.001f) {
        state.activeTab = "main";
        state.prevTab   = "";
        state.tabT      = 1.0f;
        return Box({0, 0}, {});
    }

    // Tab slide animation — cubic-out easing
    const float tabSpeed = 6.0f;
    if (state.tabT < 1.0f)
        state.tabT = std::min(state.tabT + dt * tabSpeed, 1.0f);

    float inv     = 1.0f - state.tabT;
    float tabEase = 1.0f - inv * inv * inv;

    const int drawerWidth   = 450;
    const int contentWidth  = drawerWidth - 80;
    const int contentHeight = wh - 80;

    bool transitioning = (state.tabT < 1.0f)
                      && !state.prevTab.empty()
                      && tabs.find(state.prevTab) != tabs.end();

    // style.position offsets: outgoing slides left, incoming slides from right
    int incomingX = (contentWidth  * (1 - tabEase));
    int outgoingX = (contentWidth * tabEase);

    // Build tab content
    Widget tabContent;
    if (transitioning) {
        Widget outgoing = tabs.at(state.prevTab)();
        Widget incoming = tabs.at(state.activeTab)();

        // Apply style.position directly — widgets keep their real size for layout
        outgoing.style.position = {outgoingX, 0};
        outgoing.size = {drawerWidth, wh};
        incoming.style.position = {incomingX, 0};
        incoming.size = {drawerWidth, wh};

        // Both live in the same Clip so they're cut off at the content boundary
        tabContent = Clip(id + "_tabclip", {contentWidth, contentHeight}, 0,
            Stack({
                outgoing,
                incoming,
            })
        );
    } else {
        tabContent = Expanded(1, 1, tabs.at(state.activeTab)());
    }

    // Drawer slide-in from right
    float drawerEase = 1.0f - (1.0f - state.t) * (1.0f - state.t);
    int xPos = ww - (int)(drawerWidth * drawerEase);

    return Position(PositionType::Fixed, {xPos, 0},
        BackdropBlur(40, {
            RoundedBox({drawerWidth, wh}, {.color = drawerBg}, {
                Expanded(1, 1,
                    Padding({40, 40},
                        Expanded(1, 1, tabContent)
                    )
                )
            })
        })
    );
}