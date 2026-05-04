#pragma once

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Vector2 { int x, y; };

Vector2 LerpVector(Vector2 a, Vector2 b, float t);

namespace Easing {
    inline float Linear(float t) { return t; }
    inline float EaseInQuad(float t) { return t * t; }
    inline float EaseOutQuad(float t) { return t * (2 - t); }
    inline float EaseInOutQuad(float t) { return t < 0.5f ? 2 * t * t : -1 + (4 - 2 * t) * t; }
    inline float EaseOutBack(float t) { const float c1 = 1.70158f; const float c3 = c1 + 1; return 1 + c3 * std::pow(t - 1, 3) + c1 * std::pow(t - 1, 2); }
}