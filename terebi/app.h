#pragma once

#include "../components/shared.h"
#include "../nimble/application.h"
#include "../screens/home_page.h"
#include "../utils/theme.h"

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
        fonts_.arial28 = Nimble::LoadFont("assets/fonts/Arial.ttf", 28);
        fonts_.fontAwesome24 = Nimble::LoadFont("fontawesome/fa-solid-900.otf", 24);
        fonts_.fontAwesomeB24 = Nimble::LoadFont("fontawesome/fa-brands-400.otf", 24);

        context_.settingsOpen = true;
        context_.wifiToggled = static_cast<bool>(nmcli::enabled);
        context_.currentTheme = WalLoadTheme();
        context_.pywalEnabled = false;
        g_NextFocusedWidgetId = "navbar_home";
    }

    Widget Build(int width, int height, float deltaTime) override
    {
        using namespace Widgets;
        Widget page = HomePage(width, height, deltaTime, searchString_, fonts_, context_);


        return page;
    }

private:
    AppFonts fonts_ = {};
    GlobalContext context_ = {};
    std::string searchString_;
};
