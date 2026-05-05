#pragma once
#include "../sdl_ui_kit/utils/helpers.h"

namespace FontAwesome {
    // Basic Navigation
    inline IconData Home()             { return { 0xf015 }; }
    inline IconData Search()           { return { 0xf002 }; }
    inline IconData Cog()              { return { 0xf013 }; }
    inline IconData Cogs()             { return { 0xf085 }; } // System
    inline IconData User()             { return { 0xf007 }; }
    inline IconData Bell()             { return { 0xf0f3 }; } // Notifications
    inline IconData Info()             { return { 0xf05a }; }

    // Media & Content (Perfect for "terebi")
    inline IconData Film()             { return { 0xf008 }; } // Movies
    inline IconData Tv()               { return { 0xf26c }; } // Shows
    inline IconData Play()             { return { 0xf04b }; }
    inline IconData PlayCircle()       { return { 0xf144 }; }
    inline IconData Pause()            { return { 0xf04c }; }
    inline IconData Stop()             { return { 0xf04d }; }
    inline IconData Library()          { return { 0xf02d }; } // Book/Library
    inline IconData Folder()           { return { 0xf07b }; }
    inline IconData Images()           { return { 0xf302 }; } // Photos
    inline IconData Download()         { return { 0xf019 }; }
    inline IconData Grid()             { return { 0xf009 }; } // Apps/Th-Large
    inline IconData List()             { return { 0xf03a }; }

    // Settings / UI specific
    inline IconData Desktop()          { return { 0xf108 }; } // Display
    inline IconData PaintBrush()       { return { 0xf1fc }; } // Interface/Appearance
    inline IconData UniversalAccess()  { return { 0xf29a }; } // Accessibility

    // Interaction & Status
    inline IconData Heart()            { return { 0xf004 }; } // Favorites
    inline IconData Star()             { return { 0xf005 }; } // Rating
    inline IconData Plus()             { return { 0xf067 }; } // Add to list
    inline IconData Check()            { return { 0xf00c }; }
    inline IconData Times()            { return { 0xf00d }; } // Close
    inline IconData Trash()            { return { 0xf1f8 }; }
    inline IconData Share()            { return { 0xf064 }; }

    // Directions
    inline IconData ChevronLeft()      { return { 0xf053 }; }
    inline IconData ChevronRight()     { return { 0xf054 }; }
    inline IconData ChevronUp()        { return { 0xf077 }; }
    inline IconData ChevronDown()      { return { 0xf078 }; }
    inline IconData ArrowLeft()        { return { 0xf060 }; }
    inline IconData ArrowRight()       { return { 0xf061 }; }

    // System
    inline IconData Wifi()             { return { 0xf1eb }; }
    inline IconData BatteryFull()      { return { 0xf240 }; }
    inline IconData VolumeUp()         { return { 0xf028 }; }
    inline IconData VolumeMute()       { return { 0xf6a9 }; }
    inline IconData PowerOff()         { return { 0xf011 }; }
    inline IconData EllipsisH()        { return { 0xf141 }; } // More menu
}