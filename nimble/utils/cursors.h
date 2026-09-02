#pragma once
#include <SDL3/SDL.h>

enum class CursorType
{
    Arrow,
    Hand,
    Wait,
    IBeam,
    Crosshair,
    SizeNS,
    SizeWE,
    SizeNWSE,
    SizeNESW,
    SizeAll,
    No,
    WaitArrow
};

// Initialize cursors (call once after SDL_Init)
void Cursors_Init();

// Set cursor
void SetCursor(CursorType type);

// Reset to default arrow cursor
void ResetCursor();

// Optional: Set custom cursor
void SetCustomCursor(SDL_Cursor* cursor);

// Cleanup (call before SDL_Quit)
void Cursors_Quit();