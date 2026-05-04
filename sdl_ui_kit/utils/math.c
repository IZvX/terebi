#pragma once

inline Vector2 LerpVector(Vector2 a, Vector2 b, float t) { 
    return { (int)(a.x + (b.x - a.x) * t), (int)(a.y + (b.y - a.y) * t) }; 
}