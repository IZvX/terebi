#pragma once
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include "../utils/wal.h"
#include "../utils/theme.h"

// Group our loaded fonts
struct AppFonts {
    TTF_Font* spaceGrotesk12;
    TTF_Font* spaceGrotesk24;
    TTF_Font* spaceGrotesk48;
    TTF_Font* arial12;
    TTF_Font* arial14;
    TTF_Font* arial16;
    TTF_Font* arial18;
    TTF_Font* arial20;
    TTF_Font* arial28;
    TTF_Font* fontAwesome24;
    TTF_Font* fontAwesomeB24;
};

// Application State
struct GlobalContext {
    bool settingsOpen = false;
    bool wifiToggled = false;
    std::string searchString = "";
    bool pywalEnabled = true;
    WalTheme currentTheme = {};
};

// C++17 inline allows this to be shared across all files including this header
inline GlobalContext g_Context;

// Helper to reload pywal theme
inline void ReloadPywalTheme() {
    g_Context.currentTheme = WalLoadTheme();
}

// Helper to update context
inline void setGlobalContext(const GlobalContext& newCtx) {
    g_Context = newCtx;
}