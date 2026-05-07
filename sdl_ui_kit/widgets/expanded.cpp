#pragma once
namespace Widgets {
    inline Widget Expanded(float vertical, float horizontal, Widget child) {
        child.expandY = std::max(0.0f, vertical);   // removed clamp upper bound — Row uses expandX as flex weight (can be > 1)
        child.expandX = std::max(0.0f, horizontal); // same
        return child;
    }
}