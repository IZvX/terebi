#pragma once
namespace Widgets {
    inline Widget Expanded(float vertical, float horizontal, Widget child) {
        child.expandY = std::clamp(vertical, 0.0f, 1.0f);
        child.expandX = std::clamp(horizontal, 0.0f, 1.0f);
        return child;
    }
}