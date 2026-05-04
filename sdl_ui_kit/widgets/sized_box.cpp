#pragma once
namespace Widgets {
    inline Widget SizedBox(Vector2 size) { 
        return {"", size, {}, {}, {},[](SDL_Renderer*, SDL_Rect, const WidgetStyle&, const InputState&, const std::vector<Widget>&, WidgetDebug){}}; 
    }
}