#pragma once
#include "../shared.h"
#include "../nav_item_rounded.h"
#include "../settings_drawer.h"
#include "../drawer_toggle.h"
#include "../drawer.h"
#include "../../utils/nmcli.h"

#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <functional>
#include <algorithm>

// -----------------------------------------------------------------------------
// Shared State for the Network Tabs
// -----------------------------------------------------------------------------
namespace NetworkTabState {
    inline std::vector<nmcli::Network> networks;
    inline bool isScanning  = false;
    inline bool hasScanned  = false;
    inline std::mutex networkMutex;

    inline nmcli::Network selectedNetwork;
    inline std::string passwordBuffer = "";
}

// -----------------------------------------------------------------------------
// UI Components
// -----------------------------------------------------------------------------

inline Widget NetworkDialogButton(
    const std::string& label,
    const std::string& id,
    const std::string& up, const std::string& down,
    const std::string& left, const std::string& right,
    std::function<void()> onClick,
    AppFonts& fonts,
    bool isPrimary = false)
{
    using namespace Widgets;

    WidgetStyle style;
    style.color  = isPrimary ? SDL_Color{100, 210, 160, 255} : SDL_Color{255, 255, 255, 25};
    style.radius = 6;

    SDL_Color textColor = isPrimary ? SDL_Color{0, 0, 0, 255} : SDL_Color{255, 255, 255, 255};

    // width=0 so Column Stretch fills it; height=44 gives a real intrinsic height
    // so nothing collapses when there is no Expanded flex space above
    Widget w = RoundedBox({0, 44}, style,
        Padding({20, 10}, Text(label, fonts.arial18, textColor, {}))
    , {-1, 0});
    w.id = id;

    w.OnKeyPress(SDLK_RETURN, onClick);
    w.OnClick(id + "_click", 0, onClick, [](Widget&, float){});

    // No Expanded wrapper — height is fixed (44), width comes from Column Stretch
    return w
        .OnFocus(id, 0.2f, [](Widget &w, float t) {
            w.animateBorder({255, 255, 255, 255}, 2, t);
        })
        .OnHover(id, 0.2f, [](Widget &w, float t) {
            SetCursor(CursorType::Hand);
            w.animateBorder({255, 255, 255, 255}, 2, t);
        })
        .WithNav(up, down, left, right, "", "");
}

inline Widget DrawerItem(
    uint32_t iconCode,
    const std::string &label,
    const std::string &id,
    const WidgetNav &nav,
    std::function<void()> onClick,
    AppFonts& fonts)
{
    using namespace Widgets;

    WidgetStyle style;
    style.color  = {255, 255, 255, 8};
    style.radius = 12;

    // width=0 so Column Stretch fills it; height intrinsic from Padding+content
    Widget w = RoundedBox({0, 0}, style,
        Padding({20, 18},
            Row(MainAxisAlignment::Start, CrossAxisAlignment::Center, 20, {
                Icon({iconCode}, fonts.fontAwesome24, 24, {161, 161, 170, 255}),
                Text(label, fonts.arial18, {161, 161, 170, 255})
            })
        ),
        {-1, 0}   // left-align content inside the box
    );

    w.id = id;
    w.OnClick(id + "_click", 0.0f, onClick, [](Widget&, float){});
    w.OnKeyPress(SDLK_RETURN, onClick);

    // No Expanded — height is intrinsic, width from Column Stretch
    return w
        .OnHover(id, 0.25f, [](Widget &w, float t) {
            SetCursor(CursorType::Hand);
            w.animateColor({255, 255, 255, 25}, t);
            w.animateBorder({255, 255, 255, 255}, 2, t);
            w.children[0].children[0].children[0].animateColor({255, 255, 255, 255}, t);
            w.children[0].children[0].children[1].animateColor({255, 255, 255, 255}, t);
        })
        .OnFocus(id, 0.25f, [](Widget &w, float t) {
            w.animateColor({255, 255, 255, 25}, t);
            w.animateBorder({255, 255, 255, 255}, 2, t);
            w.children[0].children[0].children[0].animateColor({255, 255, 255, 255}, t);
            w.children[0].children[0].children[1].animateColor({255, 255, 255, 255}, t);
        })
        .WithNav(nav.up, nav.down, nav.left, nav.right, nav.next, nav.prev);
}

// -----------------------------------------------------------------------------
// Drawer Tabs
// -----------------------------------------------------------------------------

inline Widget SettingsTab_NetworkDetails(AppFonts& fonts) {
    using namespace Widgets;
    using namespace NetworkTabState;

    // Back nav row — intrinsic height, full width via outer Stretch
    Widget backNav = Row(MainAxisAlignment::Start, CrossAxisAlignment::Center, 15, {
        NavItemRounded(FontAwesome::ArrowLeft().codepoint, "Back", "drawer_network_details_back",
            { "", selectedNetwork.active ? "drawer_network_btn_disconnect" : "drawer_network_details_tf", "", "", "", "" }, fonts)
            .OnClick("drawer_network_details_back", 0.25f, []() {
                SetDrawerTab("settings_drawer", "network");
                g_NextFocusedWidgetId = "drawer_settings_network_toggle_wifi";
            }, [](Widget&, float){}),
        Text(selectedNetwork.ssid, fonts.arial28, {255, 255, 255, 255}, {}),
    });

    std::vector<Widget> children;

    if (selectedNetwork.active) {
        // Left-aligned status text — no centering, just Start alignment from Column
        children.push_back(
            Text("Status: Connected", fonts.arial18, {100, 210, 160, 255}, {})
        );
        // Disconnect button — height=44 fixed, width from Stretch
        children.push_back(
            NetworkDialogButton("Disconnect", "drawer_network_btn_disconnect",
                "drawer_network_details_back", "", "", "", [](){
                    std::thread([]() {
                        selectedNetwork.disconnect();
                        hasScanned = false;
                    }).detach();
                    SetDrawerTab("settings_drawer", "network");
                    g_NextFocusedWidgetId = "drawer_settings_network_toggle_wifi";
                }, fonts, false)
        );
    } else {
        // Left-aligned status text
        children.push_back(
            Text("Status: Not Connected", fonts.arial18, {161, 161, 170, 255}, {})
        );

        TextFieldStyle tfStyle;
        tfStyle.font                = fonts.arial18;
        tfStyle.backgroundColor     = {10, 10, 10, 200};
        tfStyle.borderColor         = {100, 100, 100, 255};
        tfStyle.focusedBorderColor  = {255, 255, 255, 255};
        tfStyle.textColor           = {255, 255, 255, 255};
        tfStyle.placeholderColor    = {120, 120, 120, 255};
        tfStyle.radius              = 8;
        tfStyle.padding             = {15, 10};

        // width=0 so Stretch fills it; height=44 fixed
        children.push_back(
            Widgets::TextField("drawer_network_details_tf", passwordBuffer, "Enter Password...", {0, 44}, tfStyle)
            .WithNav("drawer_network_details_back", "drawer_network_btn_connect", "", "", "", "")
        );

        // Connect — height=44 fixed, width from Stretch
        children.push_back(
            NetworkDialogButton("Connect", "drawer_network_btn_connect",
                "drawer_network_details_tf", "drawer_network_btn_cancel", "", "", [](){
                    std::thread([]() {
                        selectedNetwork.connect(passwordBuffer);
                        hasScanned = false;
                    }).detach();
                    SetDrawerTab("settings_drawer", "network");
                    g_NextFocusedWidgetId = "drawer_settings_network_toggle_wifi";
                }, fonts, true)
        );

        // Cancel — height=44 fixed, width from Stretch
        children.push_back(
            NetworkDialogButton("Cancel", "drawer_network_btn_cancel",
                "drawer_network_btn_connect", "drawer_network_details_back", "", "", [](){
                    SetDrawerTab("settings_drawer", "network");
                    g_NextFocusedWidgetId = "drawer_settings_network_toggle_wifi";
                }, fonts, false)
        );
    }

    // CrossAxisAlignment::Stretch: every child fills full panel width.
    // All children have real intrinsic heights — no Expanded, nothing collapses.
    return Column(MainAxisAlignment::Start, CrossAxisAlignment::Stretch, 20, {
        backNav,
        Column(MainAxisAlignment::Start, CrossAxisAlignment::Stretch, 15, children)
    });
}

inline Widget SettingsTab_Network(AppFonts& fonts) {
    using namespace Widgets;
    using namespace NetworkTabState;

    if (g_Context.wifiToggled && !hasScanned && !isScanning) {
        isScanning = true;
        std::thread([]() {
            auto nets = nmcli::list_networks();
            std::lock_guard<std::mutex> lock(networkMutex);
            networks   = nets;
            isScanning = false;
            hasScanned = true;
        }).detach();
    }

    if (!g_Context.wifiToggled && hasScanned) {
        std::lock_guard<std::mutex> lock(networkMutex);
        networks.clear();
        hasScanned = false;
    }

    auto getNavId = [](int i) { return "drawer_network_item_" + std::to_string(i); };

    std::vector<Widget> listItems;

    std::string toggleDown = (g_Context.wifiToggled && !isScanning) ? "drawer_network_btn_scan" : "";

    listItems.push_back(DrawerToggle(
        g_Context.wifiToggled, FontAwesome::Wifi().codepoint, "WiFi",
        "drawer_settings_network_toggle_wifi",
        { "drawer_network_back", toggleDown, "", "", "", "" },
        fonts, [](bool newState) { std::thread([newState]() { nmcli::enabled = newState; }).detach(); }
    ));

    if (g_Context.wifiToggled) {
        std::lock_guard<std::mutex> lock(networkMutex);
        if (isScanning) {
            listItems.push_back(
                Text("Scanning...", fonts.arial18, {161, 161, 170, 255}, {})
            );
        } else {
            WidgetStyle scanStyle;
            scanStyle.color  = {255, 255, 255, 50};
            scanStyle.radius = 12;

            Widget scanButton = RoundedBox({24, 24}, scanStyle,
                {Icon({FontAwesome::Reload().codepoint}, fonts.fontAwesome24, 12, {161, 161, 170, 255})});
            scanButton.id = "drawer_network_btn_scan";

            scanButton.OnClick("drawer_network_btn_scan", 0.0f, []() {
                hasScanned = false;
            }, [](Widget&, float){});
            scanButton.OnKeyPress(SDLK_RETURN, []() {
                hasScanned = false;
            });
            scanButton = scanButton
                .OnHover("drawer_network_btn_scan", 0.2f, [](Widget& w, float t) {
                    SetCursor(CursorType::Hand);
                    w.animateColor({255, 255, 255, 100}, t);
                })
                .OnFocus("drawer_network_btn_scan", 0.2f, [](Widget& w, float t) {
                    w.animateBorder({255, 255, 255, 255}, 1, t);
                })
                .WithNav("drawer_settings_network_toggle_wifi", networks.empty() ? "" : getNavId(0), "", "");

            // "Available Networks" label row — left-aligned, intrinsic height
            listItems.push_back(
                Row(MainAxisAlignment::Start, CrossAxisAlignment::Center, 10, {
                    Text("Available Networks", fonts.arial18, {255, 255, 255, 255}),
                    scanButton
                })
            );

            for (int i = 0; i < (int)networks.size(); ++i) {
                const auto& net     = networks[i];
                std::string navId   = getNavId(i);
                std::string upNav   = (i == 0)                     ? "drawer_network_btn_scan" : getNavId(i - 1);
                std::string downNav = (i == (int)networks.size()-1) ? ""                        : getNavId(i + 1);
                std::string itemLabel = net.ssid + (net.active ? " (Connected)" : "");

                // DrawerItem has no Expanded — height intrinsic, width from outer Column Stretch
                listItems.push_back(
                    DrawerItem(FontAwesome::Wifi().codepoint, itemLabel, navId,
                        {upNav, downNav, "", "", "", ""},
                        [net]() {
                            selectedNetwork = net;
                            passwordBuffer  = "";
                            SetDrawerTab("settings_drawer", "network_details");
                            g_NextFocusedWidgetId = "drawer_network_details_back";
                        }, fonts)
                );
            }
        }
    }

    // Back nav row
    Widget backNav = Row(MainAxisAlignment::Start, CrossAxisAlignment::Center, 15, {
        NavItemRounded(FontAwesome::ArrowLeft().codepoint, "Back", "drawer_network_back",
            { "", "drawer_settings_network_toggle_wifi", "", "", "", "" }, fonts)
            .OnClick("drawer_network_back", 0.25f, []() {
                SetDrawerTab("settings_drawer", "main");
                g_NextFocusedWidgetId = "drawer_settings_main_network";
            }, [](Widget&, float){}),
        Text("Network & Internet", fonts.arial28, {255, 255, 255, 255}, {}),
    });

    // Scrollable list column — Stretch gives each item full width,
    // scroll handles overflow when there are many networks
    return Column(MainAxisAlignment::Start, CrossAxisAlignment::Stretch, 20, {
        backNav,
        Column(MainAxisAlignment::Start, CrossAxisAlignment::Stretch, 10, listItems,
            ScrollBehavior::Auto, ScrollAxis::Vertical, "network_list_scroll")
    });
}