#pragma once

inline SDL_Color LerpColor(SDL_Color a, SDL_Color b, float t) {
    return {
        (Uint8)(a.r + (b.r - a.r) * t), 
        (Uint8)(a.g + (b.g - a.g) * t),
        (Uint8)(a.b + (b.b - a.b) * t), 
        (Uint8)(a.a + (b.a - a.a) * t)
    };
}