#pragma once
#include <string>
#include <unordered_map>

struct WidgetOverrides {
    bool hasExpandX = false; float expandX = 0.0f;
    bool hasExpandY = false; float expandY = 0.0f;
    bool hasColor = false; SDL_Color color = {0, 0, 0, 0};
    bool hasRadius = false; int radius = 0;
    bool hasPadding = false; Vector2 padding = {0, 0};
    bool hasDebugBounds = false; bool showBounds = false;
    void Reset() { *this = WidgetOverrides(); }
};

inline std::unordered_map<std::string, WidgetOverrides> g_InspectorOverrides;

inline void DrawWidgetInspector(Widget &widget, std::string parentPath = "root", int index = 0)
{
    // Keeping your original DrawWidgetInspector body verbatim...
    // (You can copy-paste the body of DrawWidgetInspector here exactly as you had it!)
}