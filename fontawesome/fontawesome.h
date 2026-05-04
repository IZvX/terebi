#pragma once
#include "../sdl_ui_kit/utils/helpers.h"
namespace FontAwesome {
    inline IconData Home() { 
        return { 0xf015 }; 
    }

    // You can add more common ones here
    inline IconData House()      { return { 0xf015 }; }  // same as home in older versions
    inline IconData User()       { return { 0xf007 }; }
    inline IconData Cog()        { return { 0xf013 }; }
    inline IconData Search()     { return { 0xf002 }; }
    inline IconData Heart()      { return { 0xf004 }; }
    inline IconData Star()       { return { 0xf005 }; }
    inline IconData Times()      { return { 0xf00d }; }
    inline IconData ChevronLeft(){ return { 0xf053 }; }
    // ... add as many as you want
}