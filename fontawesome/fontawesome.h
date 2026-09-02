#pragma once
#include "../nimble/utils/helpers.h"

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
    inline IconData Reload()           { return { 0xf021 }; } // Added: Sync / Update
    inline IconData Wifi()             { return { 0xf1eb }; }
    inline IconData BatteryFull()      { return { 0xf240 }; }
    inline IconData VolumeUp()         { return { 0xf028 }; }
    inline IconData VolumeMute()       { return { 0xf6a9 }; }
    inline IconData PowerOff()         { return { 0xf011 }; }
    inline IconData EllipsisH()        { return { 0xf141 }; } // More menu

    // =====================================================================
    // BRANDS (Requires fontawesomebrands-400.otf)
    // =====================================================================

    // Requested
    inline IconData Github()           { return { 0xf09b }; }
    inline IconData Instagram()        { return { 0xf16d }; }
    inline IconData Discord()          { return { 0xf392 }; }

    // Social & Communication
    inline IconData Twitter()          { return { 0xf099 }; }
    inline IconData Facebook()         { return { 0xf09a }; }
    inline IconData YouTube()          { return { 0xf167 }; }
    inline IconData Twitch()           { return { 0xf1e8 }; }
    inline IconData Reddit()           { return { 0xf1a1 }; }
    inline IconData WhatsApp()         { return { 0xf232 }; }
    inline IconData Telegram()         { return { 0xf2c6 }; }
    inline IconData Slack()            { return { 0xf198 }; }
    inline IconData LinkedIn()         { return { 0xf08c }; }
    inline IconData TikTok()           { return { 0xe07b }; }

    // Platforms & OS
    inline IconData Apple()            { return { 0xf179 }; }
    inline IconData Android()          { return { 0xf17b }; }
    inline IconData Windows()          { return { 0xf17a }; }
    inline IconData Linux()            { return { 0xf17c }; }
    inline IconData Ubuntu()           { return { 0xf2df }; }

    // Gaming
    inline IconData Steam()            { return { 0xf1b6 }; }
    inline IconData Xbox()             { return { 0xf412 }; }
    inline IconData PlayStation()      { return { 0xf3df }; }
    inline IconData ItchIo()           { return { 0xf83a }; }

    // Tech & Dev
    inline IconData Google()           { return { 0xf1a0 }; }
    inline IconData Git()              { return { 0xf1d3 }; }
    inline IconData Docker()           { return { 0xf395 }; }
    inline IconData StackOverflow()    { return { 0xf16c }; }
    inline IconData Npm()              { return { 0xf3d4 }; }
    inline IconData Python()           { return { 0xf3e2 }; }

    // Media & E-Commerce
    inline IconData Spotify()          { return { 0xf1bc }; }
    inline IconData Soundcloud()       { return { 0xf1be }; }
    inline IconData Amazon()           { return { 0xf270 }; }
    inline IconData Patreon()          { return { 0xf3f9 }; }
    inline IconData Paypal()           { return { 0xf1ed }; }
    inline IconData Stripe()           { return { 0xf42d }; }
    inline IconData Backspace()        { return { 0xf55a }; }
    inline IconData SpaceBar()         { return { 0xf242 }; }
    inline IconData Shift()            { return { 0xf062 }; }
    inline IconData ShiftOutline()     { return { 0xf077 }; }
    inline IconData KeyboardDown()     { return { 0xf3fa }; }
    inline IconData ArrowPointer()     { return { 0xf245 }; }
    inline IconData ICursor()          { return { 0xf246 }; }
    inline IconData HandPointer()      { return { 0xf25a }; }
    inline IconData RaspberryPi()           { return { 0xf7bb }; }

    // 
}