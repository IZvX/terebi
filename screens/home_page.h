#pragma once
#include <string>
#include "../components/shared.h"
#include "../components/nav_item.h"
#include "../components/nav_item_rounded.h"
#include "../components/search_button.h"

inline Widget HomePage(int ww, int wh, float dt, std::string& search, AppFonts& fonts)
{
    using namespace Widgets;
    WidgetStyle logoStyle;
    logoStyle.color = {0, 0, 0, 0};

    return Scaffold(
        Column(MainAxisAlignment::Start, CrossAxisAlignment::Start, 0, {
            Expanded(0, 1, Padding({60, 40}, Expanded(0, 1, Row(MainAxisAlignment::SpaceBetween, CrossAxisAlignment::Center, 0, {
                Row(MainAxisAlignment::Start, CrossAxisAlignment::Center, 30, {
                    RoundedBox({}, logoStyle, {Text("terebi", fonts.spaceGrotesk24, {255, 255, 255, 255}, {})}),
                    NavItem(FontAwesome::Home().codepoint, "Home", "navbar_home", {"", "", "navbar_settings", "navbar_movies", "navbar_movies", "navbar_settings"}, fonts),
                    NavItem(FontAwesome::Home().codepoint, "Movies", "navbar_movies", {"", "", "navbar_home", "navbar_shows", "navbar_shows", "navbar_home"}, fonts),
                    NavItem(FontAwesome::Home().codepoint, "Shows", "navbar_shows", {"", "", "navbar_movies", "navbar_apps", "navbar_apps", "navbar_movies"}, fonts),
                    NavItem(FontAwesome::Home().codepoint, "Apps", "navbar_apps", {"", "", "navbar_shows", "navbar_library", "navbar_library", "navbar_shows"}, fonts),
                    NavItem(FontAwesome::Home().codepoint, "Library", "navbar_library", {"", "", "navbar_apps", "navbar_search", "navbar_search", "navbar_apps"}, fonts),
                }),
                Row(MainAxisAlignment::Start, CrossAxisAlignment::Center, 30, {
                    SearchButton(FontAwesome::Search().codepoint, "navbar_search", {"", "", "navbar_library", "navbar_settings", "navbar_settings", "navbar_library"}, dt, search, fonts)
                        .OnClick("navbar_search", 0.25f, []() {}, [](Widget &w, float t) {
                            g_SearchState["navbar_search"].expanded = true;
                            g_SearchState["navbar_search"].focusSent = false;
                            g_NextFocusedWidgetId = "navbar_search";
                        }),
                    NavItemRounded(FontAwesome::Cog().codepoint, "Search", "navbar_settings", {"", "", "navbar_search", "navbar_home", "navbar_search", "navbar_home"}, fonts)
                })
            }))))
        }),
        ForegroundBlur(0, Stack({
            Image("assets/images/frieren.jpg", {ww, wh}),
            Box({ww, wh}, {{0, 0, 0, 205}})
        }))
    );
}