#pragma once
#include <string>
#include "../components/shared.h"
#include "../components/nav_item.h"
#include "../components/nav_item_rounded.h"
#include "../components/search_button.h"
#include "../components/drawer.h"
#include "../components/drawer_item.h"

#include "../components/settings_drawer.h" 
inline Widget HomePage(int ww, int wh, float dt, std::string& search, AppFonts& fonts, GlobalContext& gctx)
{
    using namespace Widgets;
    WidgetStyle logoStyle;
    logoStyle.color = {0, 0, 0, 0};
    bool pywalEnabled = g_Context.pywalEnabled;
    WalTheme wal = g_Context.currentTheme;

    SDL_Color navbarBg = GetThemeColor(
        DefaultTheme::BackgroundSecondary,
        wal.background,
        pywalEnabled
    );

    navbarBg.a = 125;

    Widget nav = Row(MainAxisAlignment::SpaceBetween, CrossAxisAlignment::Center, 0, {
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
                    NavItemRounded(FontAwesome::Cog().codepoint, "Search", "navbar_settings", {"", "", "navbar_search", "navbar_home", "navbar_home", "navbar_search"}, fonts)
                        .OnClick("navbar_settings", 0.25f, []() {g_Context.settingsOpen = !g_Context.settingsOpen; g_NextFocusedWidgetId = "drawer_settings_closesettings";}, [](Widget &w, float t) {
                            
                        })
                })
            },ScrollBehavior::None,ScrollAxis::Horizontal,"",1.0f,0.0f);
    Widget navWrapper = Expanded(0, 1, Box({0,0},{.color = navbarBg},Padding({60, 40},Expanded(0, 1, nav))));


    return Scaffold(
            Column(MainAxisAlignment::Start, CrossAxisAlignment::Start, 0, {
                navWrapper,
                RenderSettingsDrawer(ww, wh, dt, fonts),
        }),
        ForegroundBlur(0, Stack({
            Image("assets/images/frieren.jpg", {ww, wh}),
            Box({ww, wh}, {{0, 0, 0, 64}})
        }))
    );
}