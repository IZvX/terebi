#pragma once
#include "../shared.h"
#include "../nav_item_rounded.h"
#include "../settings_drawer.h"
#include "../drawer_toggle.h"
#include "../../utils/nmcli.h"
#include <thread>
#include <mutex>
#include <vector>

// Helper to build a focusable list item for a WiFi network
inline Widget NetworkListItem(const nmcli::Network& net, int index, int totalCount, AppFonts& fonts) {
    using namespace Widgets;
    
    // Dynamically build IDs and Navigation Links for the TV Remote
    std::string id = "drawer_network_item_" + std::to_string(index);

    std::string text;
    
    // UP: If first item, go back to the WiFi Toggle. Otherwise, go to the previous network item.
    std::string upNav = (index == 0) ? "drawer_settings_network_toggle_wifi" : "drawer_network_item_" + std::to_string(index - 1);
    
    // DOWN: If last item, don't go anywhere. Otherwise, go to the next network item.
    std::string downNav = (index == totalCount - 1) ? "" : "drawer_network_item_" + std::to_string(index + 1);

    WidgetStyle style;
    style.color = {255, 255, 255, 8};
    style.radius = 12;

    Widget w = RoundedBox(
        {}, 
        style,
        Expanded(1, 0, Padding({20, 18},
            Column(MainAxisAlignment::Center,CrossAxisAlignment::Center,20,{
                Row(
                    MainAxisAlignment::SpaceBetween, CrossAxisAlignment::Center, 20, {
                        Row(
                            MainAxisAlignment::Start, CrossAxisAlignment::Center, 20, {
                                // Signal strength could dynamically change the icon later
                                Icon({FontAwesome::Wifi().codepoint}, fonts.fontAwesome24, 24, {161, 161, 170, 255}),
                                Text(net.ssid, fonts.arial18, {161, 161, 170, 255})
                            }
                        ),
                        // Show a green "Connected" badge if active
                        net.active ? Text("Connected", fonts.arial18, {100, 210, 160, 255}) : SizedBox({})
                    }
                ),
                TextField("network_connect_"+index,text,"password",{20,40},{})
            })
        )),
        {-1, 0}
    );

    w.id = id;

    // Handle TV Remote "OK" / "Enter" Press
    w.OnKeyPress(SDLK_RETURN,[net]() {
        if (!net.active) {
            std::cout << "TV OS: User clicked network " << net.ssid << " - Time to show a password modal!" << std::endl;
            // TODO: Set a state here to show an OS Keyboard/Password Input Modal
        }
    });

    // Handle Mouse Clicks (just in case)
    w.OnClick(id + "_click", 0, [net]() {
        if (!net.active) {
            std::cout << "TV OS: User clicked network " << net.ssid << std::endl;
        }
    },[](Widget&, float){});

    // Bind TV Navigation and Focus Styling
    return Expanded(0, 1, w
        .OnFocus(id, 0.2f,[](Widget &w, float t) {
            w.animateColor({255, 255, 255, 25}, t);      // Lighten Background
            w.animateBorder({255, 255, 255, 255}, 2, t); // Show TV Focus Ring
        })
        .OnHover(id, 0.2f,[](Widget &w, float t) {
            SetCursor(CursorType::Hand);
            w.animateColor({255, 255, 255, 25}, t);
            w.animateBorder({255, 255, 255, 255}, 2, t);
        })
        .WithNav(upNav, downNav, "", "", "", "") // <-- Dynamic TV remote navigation links
    );
}