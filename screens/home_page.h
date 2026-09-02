#pragma once
#include <string>
#include "../components/shared.h"
#include "../components/nav_item.h"
#include "../components/nav_item_rounded.h"
#include "../components/search_button.h"
#include "../components/drawer.h"
#include "../components/drawer_item.h"
#include "../components/settings_drawer.h" 
#include "../components/apps_section.h"
// #include "../compositor/wlroots.h"
#include "./main/main_page.h" 

inline Widget HomePage(int ww, int wh, float dt, std::string& search, AppFonts& fonts, GlobalContext& gctx)
{
    using namespace Widgets;

    WidgetStyle logoStyle;
    logoStyle.color = {0, 0, 0, 0};
    bool pywalEnabled = g_Context.pywalEnabled;
    WalTheme wal = g_Context.currentTheme;

    // Translucent glass navbar
    SDL_Color navbarBg = GetThemeColor(
        DefaultTheme::BackgroundSecondary,
        wal.background,
        pywalEnabled
    );
    navbarBg.a = 150;

    Widget nav = Row(MainAxisAlignment::SpaceBetween, CrossAxisAlignment::Center, 0, {
        Row(MainAxisAlignment::Start, CrossAxisAlignment::Center, 30, {
            RoundedBox({}, logoStyle, {Text("terebi", fonts.spaceGrotesk24, {255, 255, 255, 255}, {})}),
            NavItem(FontAwesome::Home().codepoint, "Home", "navbar_home", {"", "app_browser", "navbar_settings", "navbar_movies", "navbar_movies", "navbar_settings"}, fonts),
            NavItem(FontAwesome::Film().codepoint, "Movies", "navbar_movies", {"", "app_steam", "navbar_home", "navbar_shows", "navbar_shows", "navbar_home"}, fonts),
            NavItem(FontAwesome::Tv().codepoint, "Shows", "navbar_shows", {"", "app_youtube", "navbar_movies", "navbar_apps", "navbar_apps", "navbar_movies"}, fonts),
            NavItem(FontAwesome::Grid().codepoint, "Apps", "navbar_apps", {"", "app_spotify", "navbar_shows", "navbar_library", "navbar_library", "navbar_shows"}, fonts),
            NavItem(FontAwesome::Library().codepoint, "Library", "navbar_library", {"", "app_files", "navbar_apps", "navbar_search", "navbar_search", "navbar_apps"}, fonts),
        }),
        Row(MainAxisAlignment::Start, CrossAxisAlignment::Center, 30, {
            SearchButton(FontAwesome::Search().codepoint, "navbar_search", {"", "app_terminal", "navbar_library", "navbar_settings", "navbar_settings", "navbar_library"}, dt, search, fonts)
                .OnClick("navbar_search", 0.25f, []() {}, [](Widget &w, float t) {
                    g_SearchState["navbar_search"].expanded = true;
                    g_SearchState["navbar_search"].focusSent = false;
                    g_NextFocusedWidgetId = "navbar_search";
                }),
            NavItemRounded(FontAwesome::Cog().codepoint, "Settings", "navbar_settings", {"", "", "navbar_search", "navbar_home", "navbar_home", "navbar_search"}, fonts)
                .OnClick("navbar_settings", 0.25f, [&gctx]() {
                    gctx.settingsOpen = !gctx.settingsOpen;
                    g_Context.settingsOpen = gctx.settingsOpen;
                    // if (g_terebi_shared) g_terebi_shared->drawer_open = gctx.settingsOpen;
                    g_NextFocusedWidgetId = "drawer_settings_closesettings";
                }, [](Widget &w, float t) {})
        })
    }, ScrollBehavior::None, ScrollAxis::Horizontal, "", 1.0f, 0.0f);

    Widget navWrapper = Expanded(0, 1, Box({0, 0}, {.color = navbarBg}, Padding({60, 40}, Expanded(0, 1, nav))));

    return Scaffold(
        Column(MainAxisAlignment::Start, CrossAxisAlignment::Start, 0, {
            navWrapper.WithNav("", "", "", "", "", ""),
            
            // Floating Quick Apps & Main sections
            Expanded(1, 1, Column(ColumnArgs{
                .children = {
                    Padding({ 60, 20 }, RenderAppsSection(ww, wh, dt, fonts, gctx)),
                    Expanded(1, 1, MainPage(ww, wh, dt, search, fonts, gctx))
                }
            })),
            
            RenderSettingsDrawer(ww, wh, dt, fonts),
        }),
        // 100% Transparent background so apps and wallpaper underneath are visible
        Box({0, 0}, {{0, 0, 0, 0}})
    );
}