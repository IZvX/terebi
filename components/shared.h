#pragma once
#include <SDL2/SDL_ttf.h>
#include <string>

// Group our loaded fonts
struct AppFonts {
    TTF_Font* spaceGrotesk24;
    TTF_Font* arial14;
    TTF_Font* arial16;
    TTF_Font* arial18;
    TTF_Font* arial20;
    TTF_Font* arial28;
    TTF_Font* fontAwesome24;
};

// Application State
struct GlobalContext {
    bool settingsOpen = false;
    bool wifiToggled = false;
    std::string searchString = "";
};

// C++17 inline allows this to be shared across all files including this header
inline GlobalContext g_Context;

// Helper to update context
inline void setGlobalContext(const GlobalContext& newCtx) {
    g_Context = newCtx;
}