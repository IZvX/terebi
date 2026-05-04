#include "cursors.h"
#include <unordered_map>

static std::unordered_map<CursorType, SDL_Cursor*> g_cursorCache;
static SDL_Cursor* g_defaultCursor = nullptr;

static SDL_SystemCursor GetSDLSystemCursor(CursorType type)
{
    switch (type)
    {
        case CursorType::Arrow:      return SDL_SYSTEM_CURSOR_ARROW;
        case CursorType::Hand:       return SDL_SYSTEM_CURSOR_HAND;
        case CursorType::Wait:       return SDL_SYSTEM_CURSOR_WAIT;
        case CursorType::IBeam:      return SDL_SYSTEM_CURSOR_IBEAM;
        case CursorType::Crosshair:  return SDL_SYSTEM_CURSOR_CROSSHAIR;
        case CursorType::SizeNS:     return SDL_SYSTEM_CURSOR_SIZENS;
        case CursorType::SizeWE:     return SDL_SYSTEM_CURSOR_SIZEWE;
        case CursorType::SizeNWSE:   return SDL_SYSTEM_CURSOR_SIZENWSE;
        case CursorType::SizeNESW:   return SDL_SYSTEM_CURSOR_SIZENESW;
        case CursorType::SizeAll:    return SDL_SYSTEM_CURSOR_SIZEALL;
        case CursorType::No:         return SDL_SYSTEM_CURSOR_NO;
        case CursorType::WaitArrow:  return SDL_SYSTEM_CURSOR_WAITARROW;
        default:                     return SDL_SYSTEM_CURSOR_ARROW;
    }
}

void Cursors_Init()
{
    if (g_defaultCursor != nullptr) return;

    for (int i = 0; i <= static_cast<int>(CursorType::WaitArrow); ++i)
    {
        CursorType type = static_cast<CursorType>(i);
        SDL_Cursor* cursor = SDL_CreateSystemCursor(GetSDLSystemCursor(type));
        if (cursor)
            g_cursorCache[type] = cursor;
    }

    g_defaultCursor = g_cursorCache[CursorType::Arrow];
}

void SetCursor(CursorType type)
{
    if (g_defaultCursor == nullptr)
        Cursors_Init();

    auto it = g_cursorCache.find(type);
    SDL_Cursor* cursor = (it != g_cursorCache.end()) ? it->second : g_defaultCursor;

    SDL_SetCursor(cursor);
}

void ResetCursor()
{
    if (g_defaultCursor == nullptr)
        Cursors_Init();
    SDL_SetCursor(g_defaultCursor);
}

void SetCustomCursor(SDL_Cursor* cursor)
{
    if (cursor)
        SDL_SetCursor(cursor);
}

void Cursors_Quit()
{
    for (auto& pair : g_cursorCache)
        if (pair.second)
            SDL_FreeCursor(pair.second);

    g_cursorCache.clear();
    g_defaultCursor = nullptr;
}