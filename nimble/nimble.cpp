#pragma once

// --- Standard & External Libraries ---
#include "sdl_compat.h"
#include <SDL3_image/SDL_image.h>
#include <vector>
#include <functional>
#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>
#include <iostream>
#include <cstdint>

// --- Core Utils & Types ---
#include "colors.h"
#include "utils/math.h"
#include "utils/math.c"
#include "utils/helpers.h"
#include "utils/helpers.c"

// --- Base Widget System ---
#include "widget.h"

// --- Widgets ---
#include "widgets/sized_box.cpp"
#include "widgets/padding.cpp"
#include "widgets/center.cpp"
#include "widgets/stack.cpp"
#include "widgets/scaffold.cpp"
#include "widgets/image.cpp"
#include "widgets/svg.cpp"
#include "widgets/rounded_box.cpp"
#include "widgets/backdrop_blur.cpp"
#include "widgets/foreground_blur.cpp"
#include "widgets/text.cpp"
#include "widgets/icon.cpp"
#include "widgets/row.cpp"
#include "widgets/column.cpp"
#include "widgets/expanded.cpp"
#include "widgets/text_field.cpp"
#include "widgets/clip.cpp"
#include "widgets/position.cpp"
#include "widgets/opacity.cpp"
#include "widgets/toggle.cpp"
