#pragma once

#include "nimble.cpp"

namespace Nimble
{
    namespace Debug
    {
        inline WidgetDebug &global = g_GlobalDebug;
        inline GlobalSettings &settings = g_Settings;
        inline bool &showBounds = g_GlobalDebug.showBounds;
        inline bool &showLayout = g_GlobalDebug.showSpacing;
    }
}
