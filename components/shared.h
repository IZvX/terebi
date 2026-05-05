#pragma once
#include <SDL2/SDL_ttf.h>
#include "../fontawesome/fontawesome.h"
// Group our loaded fonts to easily pass them into components
struct AppFonts {
    TTF_Font* spaceGrotesk24;
    TTF_Font* arial18;
    TTF_Font* arial20;
    TTF_Font* fontAwesome24;
};