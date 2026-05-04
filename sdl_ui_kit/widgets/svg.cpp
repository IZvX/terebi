#pragma once
namespace Widgets {
    inline Widget Svg(std::string path, Vector2 size) { return Image(path, size, ObjectFit::Contain); }
}