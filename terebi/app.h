#pragma once

#include "../components/shared.h"
#include "../components/onscreen_keyboard.h"
#include "../nimble/application.h"
#include "../screens/home_page.h"
#include "remote_control.h"
#include "../utils/theme.h"

inline Widget RenderRemoteCursor(AppFonts &fonts)
{
    using namespace Widgets;
    auto [x, y] = TerebiRemote::RemoteCursorPosition();
    TerebiRemote::CursorStyle style = TerebiRemote::RemoteCursorStyle();

    IconData cursorIcon = FontAwesome::ArrowPointer();
    if (style == TerebiRemote::CursorStyle::IBeam)
        cursorIcon = FontAwesome::ICursor();
    else if (style == TerebiRemote::CursorStyle::Hand)
        cursorIcon = FontAwesome::HandPointer();

    Widget cursor = Icon(cursorIcon, fonts.fontAwesome24, 24, {255, 255, 255, 230});
    cursor.id = "remote_cursor";
    return Position(PositionType::Fixed, {x - 12, y - 12}, cursor);
}

class TerebiApp : public Nimble::IApp
{
public:
    void OnStart() override
    {
        fonts_.spaceGrotesk12 = Nimble::LoadFont("assets/fonts/spacegrotesk/SpaceGrotesk-700.ttf", 12);
        fonts_.spaceGrotesk24 = Nimble::LoadFont("assets/fonts/spacegrotesk/SpaceGrotesk-700.ttf", 24);
        fonts_.spaceGrotesk48 = Nimble::LoadFont("assets/fonts/spacegrotesk/SpaceGrotesk-700.ttf", 48);
        fonts_.arial12 = Nimble::LoadFont("assets/fonts/Arial.ttf", 12);
        fonts_.arial14 = Nimble::LoadFont("assets/fonts/Arial.ttf", 14);
        fonts_.arial16 = Nimble::LoadFont("assets/fonts/Arial.ttf", 16);
        fonts_.arial18 = Nimble::LoadFont("assets/fonts/Arial.ttf", 18);
        fonts_.arial20 = Nimble::LoadFont("assets/fonts/Arial.ttf", 20);
        fonts_.arial22 = Nimble::LoadFont("assets/fonts/Arial.ttf", 22);
        fonts_.arial24 = Nimble::LoadFont("assets/fonts/Arial.ttf", 24);
        fonts_.arial28 = Nimble::LoadFont("assets/fonts/Arial.ttf", 28);
        fonts_.fontAwesome24 = Nimble::LoadFont("fontawesome/fa-solid-900.otf", 24);
        fonts_.fontAwesomeB24 = Nimble::LoadFont("fontawesome/fa-brands-400.otf", 24);

        context_.settingsOpen = true;
        context_.wifiToggled = static_cast<bool>(nmcli::enabled);
        context_.remoteEnabled = false;
        context_.oskEnabled = true;
        context_.currentTheme = WalLoadTheme();
        context_.pywalEnabled = false;
        g_NextFocusedWidgetId = "navbar_home";
    }

Widget Build(int width, int height, float deltaTime) override
    {
        TerebiRemote::ProcessEvents();

        // 1. Toggle drawer on F3 / Alt+F3 / Super
        // if (g_terebi_shared && g_terebi_shared->open_drawer_requested) {
            // context_.settingsOpen = !context_.settingsOpen;
            // g_Context.settingsOpen = context_.settingsOpen;
            // g_terebi_shared->open_drawer_requested = false;
            // g_terebi_shared->drawer_open = context_.settingsOpen;

            // if (context_.settingsOpen) {
                // Focus the close button inside the drawer when opening
                // g_NextFocusedWidgetId = "drawer_settings_closesettings";
            // } else {
                // Return focus to the navbar when closing
                // g_NextFocusedWidgetId = "navbar_home";
            // }
        // }

        // Keep both context state copies strictly in sync
        context_.settingsOpen = g_Context.settingsOpen;
        // if (g_terebi_shared) {
            // g_terebi_shared->drawer_open = context_.settingsOpen;
        // }

        using namespace Widgets;
        Widget page = HomePage(width, height, deltaTime, searchString_, fonts_, context_);

        if (g_Context.oskEnabled && g_Context.oskVisible)
        {
            Nimble::AddLayer(RenderOnScreenKeyboard(width, height, fonts_), "osk");
            if (Nimble::SecondaryFocusedWidget().empty())
                Nimble::SetSecondaryFocus("osk_r0c0");
        }
        if (g_Context.remotePairPopup)
            Nimble::AddLayer(RenderRemotePairPopup(width, height, fonts_, TerebiRemote::GetStatus().pairingCode), "remote_pair");

        if (TerebiRemote::IsRemotePointerActive())
            Nimble::AddLayer(RenderRemoteCursor(fonts_), "remote_cursor");

        return page;
    }
private:
    AppFonts fonts_ = {};
    GlobalContext context_ = {};
    std::string searchString_;
};
