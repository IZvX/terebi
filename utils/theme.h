#pragma once
#include <SDL3/SDL.h>

// Default color palette (Nova OS-like monochrome theme)
namespace DefaultTheme {
    // Background and surface colors
    constexpr SDL_Color Background = {30, 30, 35, 255};      // Dark background
    constexpr SDL_Color BackgroundSecondary = {15, 17, 21, 217};      // Dark background
    constexpr SDL_Color Surface = {45, 45, 52, 255};         // Surface
    constexpr SDL_Color SurfaceLight = {255, 255, 255, 8};   // Very translucent white
    constexpr SDL_Color SurfaceLighter = {255, 255, 255, 25}; // Light translucent white
    
    // Text colors
    constexpr SDL_Color TextPrimary = {255, 255, 255, 255};  // Pure white
    constexpr SDL_Color TextSecondary = {161, 161, 170, 255}; // Gray
    constexpr SDL_Color TextMuted = {100, 100, 110, 255};    // Darker gray
    
    // Accent colors
    constexpr SDL_Color AccentPrimary = {1, 77, 78, 255}; // Cyan/Teal
    constexpr SDL_Color AccentSecondary = {255, 100, 100, 255}; // Red
    constexpr SDL_Color AccentTertiary = {100, 100, 210, 255}; // Blue
    
    // Interactive colors
    constexpr SDL_Color Border = {255, 255, 255, 255};        // White border
    constexpr SDL_Color Hover = {255, 255, 255, 25};
    constexpr SDL_Color Focus = {255, 255, 255, 25};
    constexpr SDL_Color Disabled = {161, 161, 170, 255};
}

// Helper function to get theme color based on pywal state
// Parameters: default color, pywal color, whether pywal is enabled, whether to use pywal
inline SDL_Color GetThemeColor(SDL_Color defaultColor, SDL_Color pywalColor, bool pywalEnabled, bool usePywal = true) {
    // If pywal is disabled, always use default
    if (!pywalEnabled) {
        return defaultColor;
    }
    
    // If pywal is enabled but the pywal color is fully transparent/uninitialized, use default
    if (usePywal && pywalColor.a == 0 && pywalColor.r == 0 && pywalColor.g == 0 && pywalColor.b == 0) {
        return defaultColor;
    }
    
    return usePywal ? pywalColor : defaultColor;
}
