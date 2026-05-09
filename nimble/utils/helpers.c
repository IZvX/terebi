#pragma once
#include "../widget.h"
inline void UpdateUIAnimations(float dt)
{
  for (auto &pair : g_UIState)
  {
    auto &anim = pair.second;

    // --- Hover Animation ---
    if (anim.hoverDuration > 0.001f)
    {
      if (anim.isHovered)
        anim.hoverProgress += dt / anim.hoverDuration;
      else
        anim.hoverProgress -= dt / anim.hoverDuration;
      anim.hoverProgress = std::clamp(anim.hoverProgress, 0.0f, 1.0f);
    }

    // --- Focus Animation (New) ---
    if (anim.focusDuration > 0.001f)
    {
      if (anim.isFocused)
        anim.focusProgress += dt / anim.focusDuration;
      else
        anim.focusProgress -= dt / anim.focusDuration;
      anim.focusProgress = std::clamp(anim.focusProgress, 0.0f, 1.0f);
    }

    if (anim.manualDuration > 0.001f)
    {
      if (anim.isManuallyAnimated)
        anim.manualProgress += dt / anim.manualDuration;
      else
        anim.manualProgress -= dt / anim.manualDuration;
        
      anim.manualProgress = std::clamp(anim.manualProgress, 0.0f, 1.0f);
    }

    // --- Click Animation ---
    if (anim.isClicked)
    {
      anim.clickProgress = 1.0f;
      anim.isClicked = false;
    }
    if (anim.clickProgress > 0.0f && anim.clickDuration > 0.001f)
    {
      anim.clickProgress -= dt / anim.clickDuration;
      anim.clickProgress = std::clamp(anim.clickProgress, 0.0f, 1.0f);
    }
  }

  // Smooth scrolling lerp (Unchanged)
  for (auto &pair : g_ScrollState)
  {
    pair.second.scrollX += (pair.second.targetX - pair.second.scrollX) * 15.0f * dt;
    pair.second.scrollY += (pair.second.targetY - pair.second.scrollY) * 15.0f * dt;
    if (std::abs(pair.second.targetX - pair.second.scrollX) < 0.5f)
      pair.second.scrollX = pair.second.targetX;
    if (std::abs(pair.second.targetY - pair.second.scrollY) < 0.5f)
      pair.second.scrollY = pair.second.targetY;
  }
}

inline void StartUIFrame()
{
  // Commit focus changes from previous frame
  g_FocusedWidgetId = g_NextFocusedWidgetId;

  // Clear the debug rects map so it's fresh for this frame's layout
  g_WidgetRects.clear();
  g_DebugNavDrawQueue.clear();
}

inline void ApplySurfaceGradient(SDL_Surface *surface, const Gradient &grad)
{
  if (!grad.enabled || !surface)
    return;
  SDL_LockSurface(surface);
  Uint32 *pixels = (Uint32 *)surface->pixels;
  int w = surface->w, h = surface->h;
  SDL_PixelFormat *fmt = surface->format;

  float angleRad = grad.angle * (M_PI / 180.0f);
  float dx = std::cos(angleRad), dy = std::sin(angleRad);
  float cx = w / 2.0f, cy = h / 2.0f;

  for (int y = 0; y < h; ++y)
  {
    for (int x = 0; x < w; ++x)
    {
      Uint8 r, g, b, a;
      SDL_GetRGBA(pixels[y * w + x], fmt, &r, &g, &b, &a);
      if (a == 0)
        continue;

      float t = 0.0f;
      if (grad.isRadial)
      {
        float dist = std::sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));
        t = std::min(1.0f, dist / (std::max(w, h) / 2.0f));
      }
      else
      {
        float proj = (x - cx) * dx + (y - cy) * dy;
        float maxProj = (w * std::abs(dx) + h * std::abs(dy)) / 2.0f;
        float denom = 2.0f * maxProj;
        if (denom < 0.0001f)
          denom = 0.0001f;
        t = std::clamp((proj + maxProj) / denom, 0.0f, 1.0f);
      }

      SDL_Color c = LerpColor(grad.startColor, grad.endColor, t);
      pixels[y * w + x] = SDL_MapRGBA(fmt, (r * c.r) / 255, (g * c.g) / 255, (b * c.b) / 255, (a * c.a) / 255);
    }
  }
  SDL_UnlockSurface(surface);
}

// Shader cache
struct ShadowCacheKey
{
  int w, h, radius, spread, blur;
  bool operator == (const ShadowCacheKey &o) const { return w == o.w && h == o.h && radius == o.radius && spread == o.spread && blur == o.blur; }
};
struct ShadowCacheHash
{
  std::size_t operator()(const ShadowCacheKey &k) const
  {
    return ((std::hash<int>()(k.w) ^ (std::hash<int>()(k.h) << 1)) >> 1) ^ (std::hash<int>()(k.radius) << 1) ^ (std::hash<int>()(k.spread) << 2) ^ (std::hash<int>()(k.blur) << 3);
  }
};

inline std::unordered_map<ShadowCacheKey, SDL_Texture *, ShadowCacheHash> g_ShadowCache;

inline SDL_Texture *GetShadowTexture(SDL_Renderer *renderer, int w, int h, int radius, int spread, int blur)
{
  ShadowCacheKey key = {w, h, radius, spread, blur};
  if (g_ShadowCache.find(key) != g_ShadowCache.end())
    return g_ShadowCache[key];

  if (g_ShadowCache.size() > 200)
  {
    for (auto &pair : g_ShadowCache)
      SDL_DestroyTexture(pair.second);
    g_ShadowCache.clear();
  }

  int pad = blur + spread;
  int tw = w + pad * 2, th = h + pad * 2;
  float scale = (tw > 256 || th > 256) ? 0.5f : 1.0f;

  int gen_w = std::max(1, (int)(tw * scale)), gen_h = std::max(1, (int)(th * scale));
  SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, gen_w, gen_h, 32, SDL_PIXELFORMAT_RGBA32);

  float cx = gen_w / 2.0f, cy = gen_h / 2.0f;
  float box_w = (w + 2 * spread) * scale, box_h = (h + 2 * spread) * scale;
  float r = (radius + spread) * scale;
  float b = std::max(1.0f, blur * scale);
  float eff_r = std::min({r, box_w / 2.0f, box_h / 2.0f});

  Uint32 *pixels = (Uint32 *)surf->pixels;
  int pitch_pixels = surf->pitch / 4;
  for (int y = 0; y < gen_h; ++y)
  {
    for (int x = 0; x < gen_w; ++x)
    {
      float dx = std::abs(x - cx) - (box_w / 2.0f - eff_r);
      float dy = std::abs(y - cy) - (box_h / 2.0f - eff_r);
      float length = std::sqrt(std::max(dx, 0.0f) * std::max(dx, 0.0f) + std::max(dy, 0.0f) * std::max(dy, 0.0f));
      float dist = length + std::min(std::max(dx, dy), 0.0f) - eff_r;

      float t = std::clamp(0.5f - dist / (b * 2.0f), 0.0f, 1.0f);
      float alpha = t * t * (3.0f - 2.0f * t);

      pixels[y * pitch_pixels + x] = SDL_MapRGBA(surf->format, 255, 255, 255, (Uint8)(alpha * 255));
    }
  }

  SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
  SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(tex, SDL_ScaleModeLinear);
  SDL_FreeSurface(surf);

  return g_ShadowCache[key] = tex;
}


inline void BoxBlurSurface(SDL_Surface *surf, int radius)
{
  if (radius <= 0)
    return;
  int w = surf->w, h = surf->h;
  std::vector<Uint8> temp(w * h * 4);
  Uint8 *pixels = (Uint8 *)surf->pixels;
  int pitch = surf->pitch;

  std::vector<float> kernel(2 * radius + 1);
  float sigma = std::max(radius / 2.0f, 1.0f);
  float sum = 0.0f;

  for (int k = -radius; k <= radius; ++k)
  {
    float weight = std::exp(-(k * k) / (2.0f * sigma * sigma));
    kernel[k + radius] = weight;
    sum += weight;
  }
  for (float &weight : kernel)
    weight /= sum;

  for (int y = 0; y < h; ++y)
  {
    for (int x = 0; x < w; ++x)
    {
      float r = 0, g = 0, b = 0, a = 0;
      for (int k = -radius; k <= radius; ++k)
      {
        int px = std::clamp(x + k, 0, w - 1);
        int idx = y * pitch + px * 4;
        float weight = kernel[k + radius];
        r += pixels[idx] * weight;
        g += pixels[idx + 1] * weight;
        b += pixels[idx + 2] * weight;
        a += pixels[idx + 3] * weight;
      }
      int out_idx = (y * w + x) * 4;
      temp[out_idx] = static_cast<Uint8>(std::clamp(std::round(r), 0.0f, 255.0f));
      temp[out_idx + 1] = static_cast<Uint8>(std::clamp(std::round(g), 0.0f, 255.0f));
      temp[out_idx + 2] = static_cast<Uint8>(std::clamp(std::round(b), 0.0f, 255.0f));
      temp[out_idx + 3] = static_cast<Uint8>(std::clamp(std::round(a), 0.0f, 255.0f));
    }
  }

  for (int y = 0; y < h; ++y)
  {
    for (int x = 0; x < w; ++x)
    {
      float r = 0, g = 0, b = 0, a = 0;
      for (int k = -radius; k <= radius; ++k)
      {
        int py = std::clamp(y + k, 0, h - 1);
        int idx = (py * w + x) * 4;
        float weight = kernel[k + radius];
        r += temp[idx] * weight;
        g += temp[idx + 1] * weight;
        b += temp[idx + 2] * weight;
        a += temp[idx + 3] * weight;
      }
      int out_idx = y * pitch + x * 4;
      pixels[out_idx] = static_cast<Uint8>(std::clamp(std::round(r), 0.0f, 255.0f));
      pixels[out_idx + 1] = static_cast<Uint8>(std::clamp(std::round(g), 0.0f, 255.0f));
      pixels[out_idx + 2] = static_cast<Uint8>(std::clamp(std::round(b), 0.0f, 255.0f));
      pixels[out_idx + 3] = static_cast<Uint8>(std::clamp(std::round(a), 0.0f, 255.0f));
    }
  }
}

// Additive accumulation blend mode (for stacking our Gaussian passes)
inline SDL_BlendMode GetAccumulateBlendMode()
{
  static SDL_BlendMode mode = SDL_ComposeCustomBlendMode(
      SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD,
      SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
  return mode;
}

// Premultiplied blend mode (prevents dark fringes on blurred transparency)
inline SDL_BlendMode GetPremultipliedBlendMode()
{
  static SDL_BlendMode mode = SDL_ComposeCustomBlendMode(
      SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD,
      SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);
  return mode;
}

inline void RenderGPUGaussian(SDL_Renderer *renderer, SDL_Texture *srcTexture, int w, int h, float blurRadius, SDL_Rect destRect)
{
  if (blurRadius <= 0.0f)
  {
    SDL_SetTextureBlendMode(srcTexture, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(renderer, srcTexture, NULL, &destRect);
    return;
  }

  SDL_Texture *oldTarget = SDL_GetRenderTarget(renderer);

  // W3C CSS Specification: blur(radius) is exactly the Gaussian standard deviation (sigma)
  float sigma = blurRadius;
  int scale = 1;

  // Downsample aggressively for large radii to save GPU fillrate
  // This is exactly how WebKit/Blink optimizes massive CSS blurs under the hood.
  while (sigma > 3.0f)
  {
    scale *= 2;
    sigma /= 2.0f;
  }

  int dw = std::max(1, w / scale);
  int dh = std::max(1, h / scale);
  int r = std::max(1, (int)std::ceil(2.5f * sigma)); // Standard 2.5 sigma kernel limit

  // Compute true mathematical Gaussian weights
  std::vector<float> weights(r + 1);
  float sum = 0.0f;
  for (int i = 0; i <= r; ++i)
  {
    float weight = std::exp(-(i * i) / (2.0f * sigma * sigma));
    weights[i] = weight;
    sum += (i == 0) ? weight : 2.0f * weight;
  }

  // Convert weights to 255-based alpha modifiers
  std::vector<int> iWeights(r + 1);
  int iSum = 0;
  for (int i = 0; i <= r; ++i)
  {
    iWeights[i] = std::round((weights[i] / sum) * 255.0f);
    iSum += (i == 0) ? iWeights[i] : 2 * iWeights[i];
  }
  // Correct minor float rounding errors so the image doesn't randomly darken/brighten
  iWeights[0] += (255 - iSum);

  // Create GPU passes
  SDL_Texture *tDown = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, dw, dh);
  SDL_Texture *tX = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, dw, dh);
  SDL_Texture *tY = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, dw, dh);

  SDL_SetTextureScaleMode(tDown, SDL_ScaleModeLinear);
  SDL_SetTextureScaleMode(tX, SDL_ScaleModeLinear);
  SDL_SetTextureScaleMode(tY, SDL_ScaleModeLinear);

  // 1. Downscale & Premultiply Alpha
  // Rendering BLENDMODE_BLEND onto transparent black mathematically premultiplies RGB by Alpha on the GPU
  SDL_SetRenderTarget(renderer, tDown);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);
  SDL_SetTextureBlendMode(srcTexture, SDL_BLENDMODE_BLEND);
  SDL_RenderCopy(renderer, srcTexture, NULL, NULL);

  SDL_BlendMode accMode = GetAccumulateBlendMode();

  // 2. Separable Pass A (Horizontal)
  SDL_SetRenderTarget(renderer, tX);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);
  SDL_SetTextureBlendMode(tDown, accMode);

  for (int i = -r; i <= r; ++i)
  {
    int mod = iWeights[std::abs(i)];
    if (mod <= 0)
      continue;

    SDL_SetTextureColorMod(tDown, mod, mod, mod);
    SDL_SetTextureAlphaMod(tDown, mod);

    SDL_FRect dst = {(float)i, 0.0f, (float)dw, (float)dh};
    SDL_RenderCopyF(renderer, tDown, NULL, &dst);
  }

  // 3. Separable Pass B (Vertical)
  SDL_SetRenderTarget(renderer, tY);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);
  SDL_SetTextureBlendMode(tX, accMode);

  for (int i = -r; i <= r; ++i)
  {
    int mod = iWeights[std::abs(i)];
    if (mod <= 0)
      continue;

    SDL_SetTextureColorMod(tX, mod, mod, mod);
    SDL_SetTextureAlphaMod(tX, mod);

    SDL_FRect dst = {0.0f, (float)i, (float)dw, (float)dh};
    SDL_RenderCopyF(renderer, tX, NULL, &dst);
  }

  // Restore target and render perfectly Gaussian blurred result to screen
  SDL_SetRenderTarget(renderer, oldTarget);

  SDL_SetTextureColorMod(tY, 255, 255, 255);
  SDL_SetTextureAlphaMod(tY, 255);
  SDL_SetTextureBlendMode(tY, GetPremultipliedBlendMode());

  SDL_RenderCopy(renderer, tY, NULL, &destRect);

  SDL_DestroyTexture(tDown);
  SDL_DestroyTexture(tX);
  SDL_DestroyTexture(tY);
}

inline void DrawBoxOutline(SDL_Renderer *renderer, SDL_Rect rect, int borderWidth, SDL_Color color)
{
    if (rect.w <= 0 || rect.h <= 0 || borderWidth <= 0 || color.a == 0) return;
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    
    // Draw 4 rectangles OUTWARD from the edges of rect
    SDL_Rect top   = {rect.x - borderWidth, rect.y - borderWidth, rect.w + borderWidth * 2, borderWidth};
    SDL_Rect bot   = {rect.x - borderWidth, rect.y + rect.h, rect.w + borderWidth * 2, borderWidth};
    SDL_Rect left  = {rect.x - borderWidth, rect.y, borderWidth, rect.h};
    SDL_Rect right = {rect.x + rect.w, rect.y, borderWidth, rect.h};

    SDL_RenderFillRect(renderer, &top);
    SDL_RenderFillRect(renderer, &bot);
    SDL_RenderFillRect(renderer, &left);
    SDL_RenderFillRect(renderer, &right);
}

inline void DrawRoundedBoxOutlineAA(SDL_Renderer *renderer, SDL_Rect rect, int radius, int borderWidth, SDL_Color color)
{
    if (rect.w <= 0 || rect.h <= 0 || borderWidth <= 0 || color.a == 0) return;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    float half_AA = 0.5f;

    // Centers are based on the main rect
    float cx1 = rect.x + radius;
    float cx2 = rect.x + rect.w - radius;
    float cy1 = rect.y + radius;
    float cy2 = rect.y + rect.h - radius;

    if (cx1 > cx2) { cx1 = cx2 = rect.x + rect.w / 2.0f; }
    if (cy1 > cy2) { cy1 = cy2 = rect.y + rect.h / 2.0f; }

    int N = 10;
    struct Corner { float cx, cy, start_angle; };
    Corner corners[4] = {
        {cx2, cy2, 0.0f},                             // Bottom-right
        {cx1, cy2, (float)M_PI / 2.0f},               // Bottom-left
        {cx1, cy1, (float)M_PI},                      // Top-left
        {cx2, cy1, 3.0f * (float)M_PI / 2.0f}         // Top-right
    };

    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;

    SDL_Color transColor = {color.r, color.g, color.b, 0};

    auto add_vert = [&](float px, float py, SDL_Color c) {
        vertices.push_back({{px, py}, c, {0, 0}});
    };

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j <= N; ++j) 
        {
            float angle = corners[i].start_angle + (j / (float)N) * (M_PI / 2.0f);
            float c_cos = std::cos(angle);
            float c_sin = std::sin(angle);

            // CHANGED: Inner edge is exactly at `radius`, outer edge pushes out by `borderWidth`
            float r_in_aa   = std::max(0.0f, radius - half_AA);
            float r_in_sol  = radius + half_AA;
            float r_out_sol = radius + borderWidth - half_AA;
            float r_out_aa  = radius + borderWidth + half_AA;

            add_vert(corners[i].cx + r_in_aa * c_cos,  corners[i].cy + r_in_aa * c_sin,  transColor);
            add_vert(corners[i].cx + r_in_sol * c_cos, corners[i].cy + r_in_sol * c_sin, color);
            add_vert(corners[i].cx + r_out_sol * c_cos, corners[i].cy + r_out_sol * c_sin, color);
            add_vert(corners[i].cx + r_out_aa * c_cos,  corners[i].cy + r_out_aa * c_sin,  transColor);
        }
    }

    int num_sections = 4 * (N + 1);
    for (int i = 0; i < num_sections; ++i)
    {
        int next_i = (i + 1) % num_sections;
        
        int curr_in_aa   = i * 4 + 0;
        int curr_in_sol  = i * 4 + 1;
        int curr_out_sol = i * 4 + 2;
        int curr_out_aa  = i * 4 + 3;

        int next_in_aa   = next_i * 4 + 0;
        int next_in_sol  = next_i * 4 + 1;
        int next_out_sol = next_i * 4 + 2;
        int next_out_aa  = next_i * 4 + 3;

        indices.push_back(curr_in_aa); indices.push_back(next_in_aa); indices.push_back(curr_in_sol);
        indices.push_back(curr_in_sol); indices.push_back(next_in_aa); indices.push_back(next_in_sol);

        indices.push_back(curr_in_sol); indices.push_back(next_in_sol); indices.push_back(curr_out_sol);
        indices.push_back(curr_out_sol); indices.push_back(next_in_sol); indices.push_back(next_out_sol);

        indices.push_back(curr_out_sol); indices.push_back(next_out_sol); indices.push_back(curr_out_aa);
        indices.push_back(curr_out_aa); indices.push_back(next_out_sol); indices.push_back(next_out_aa);
    }

    SDL_RenderGeometry(renderer, nullptr, vertices.data(), vertices.size(), indices.data(), indices.size());
}

inline void FillRoundedBoxAA(SDL_Renderer *renderer, SDL_Rect rect, int radius, SDL_Color color, Gradient grad)
{
  if (rect.w <= 0 || rect.h <= 0 || (color.a == 0 && !grad.enabled))
    return;
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

  float half_AA = 0.5f;
  float R_in = std::max(0.0f, (float)radius - half_AA);
  float R_out = radius + half_AA;

  float cx1 = rect.x + radius, cx2 = rect.x + rect.w - radius;
  float cy1 = rect.y + radius, cy2 = rect.y + rect.h - radius;

  if (cx1 > cx2)
  {
    cx1 = cx2 = rect.x + rect.w / 2.0f;
    R_in = std::max(0.0f, rect.w / 2.0f - half_AA);
    R_out = rect.w / 2.0f + half_AA;
  }
  if (cy1 > cy2)
  {
    cy1 = cy2 = rect.y + rect.h / 2.0f;
    R_in = std::max(0.0f, rect.h / 2.0f - half_AA);
    R_out = rect.h / 2.0f + half_AA;
  }

  auto getVertColor = [&](float px, float py, float alpha_mult) -> SDL_Color
  {
    SDL_Color c = color;
    if (grad.enabled)
    {
      float t = 0;
      float cx = rect.x + rect.w / 2.0f, cy = rect.y + rect.h / 2.0f;
      if (grad.isRadial)
      {
        float dist = std::sqrt((px - cx) * (px - cx) + (py - cy) * (py - cy));
        t = std::min(1.0f, dist / (std::max(rect.w, rect.h) / 2.0f));
      }
      else
      {
        float angleRad = grad.angle * (M_PI / 180.0f);
        float dx = std::cos(angleRad), dy = std::sin(angleRad);
        float proj = (px - cx) * dx + (py - cy) * dy;
        float maxProj = (rect.w * std::abs(dx) + rect.h * std::abs(dy)) / 2.0f;
        float denom = 2.0f * maxProj;
        if (denom < 0.0001f)
          denom = 0.0001f;
        t = std::clamp((proj + maxProj) / denom, 0.0f, 1.0f);
      }
      SDL_Color gc = LerpColor(grad.startColor, grad.endColor, t);
      c = {(Uint8)((gc.r * color.r) / 255), (Uint8)((gc.g * color.g) / 255), (Uint8)((gc.b * color.b) / 255), (Uint8)((gc.a * color.a) / 255)};
    }
    c.a = (Uint8)(c.a * alpha_mult);
    return c;
  };

  std::vector<SDL_Vertex> vertices;
  std::vector<int> indices;
  float center_x = rect.x + rect.w / 2.0f, center_y = rect.y + rect.h / 2.0f;
  vertices.push_back({{center_x, center_y}, getVertColor(center_x, center_y, 1.0f), {0, 0}});
  int center_idx = 0;

  const int N = 10;
  struct Corner
  {
    float cx, cy, start_angle;
  };
  Corner corners[4] = {{cx2, cy2, 0.0f}, {cx1, cy2, (float)M_PI / 2.0f}, {cx1, cy1, (float)M_PI}, {cx2, cy1, 3.0f * (float)M_PI / 2.0f}};

  int start_idx = 1;
  for (int i = 0; i < 4; ++i)
  {
    for (int j = 0; j <= N; ++j)
    {
      float angle = corners[i].start_angle + (j / (float)N) * (M_PI / 2.0f);
      float c_cos = std::cos(angle), c_sin = std::sin(angle);
      float px_in = corners[i].cx + R_in * c_cos, py_in = corners[i].cy + R_in * c_sin;
      vertices.push_back({{px_in, py_in}, getVertColor(px_in, py_in, 1.0f), {0, 0}});
      float px_out = corners[i].cx + R_out * c_cos, py_out = corners[i].cy + R_out * c_sin;
      vertices.push_back({{px_out, py_out}, getVertColor(px_out, py_out, 0.0f), {0, 0}});
    }
  }

  int total_points = 4 * (N + 1);
  for (int i = 0; i < total_points; ++i)
  {
    int next_i = (i + 1) % total_points;
    int in_idx = start_idx + i * 2, out_idx = in_idx + 1;
    int next_in_idx = start_idx + next_i * 2, next_out_idx = next_in_idx + 1;
    indices.push_back(center_idx);
    indices.push_back(in_idx);
    indices.push_back(next_in_idx);
    indices.push_back(in_idx);
    indices.push_back(out_idx);
    indices.push_back(next_out_idx);
    indices.push_back(in_idx);
    indices.push_back(next_out_idx);
    indices.push_back(next_in_idx);
  }

  SDL_RenderGeometry(renderer, nullptr, vertices.data(), vertices.size(), indices.data(), indices.size());
}





inline void FillBox(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color color, Gradient grad = {})
{
    if (rect.w <= 0 || rect.h <= 0)
        return;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Fast path: solid color (most common case)
    if (!grad.enabled)
    {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &rect);
        return;
    }

    // Gradient path
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    std::vector<SDL_Vertex> vertices = 
    {
        {{(float)rect.x,                  (float)rect.y},                 {0,0,0,0}, {0,0}},
        {{(float)rect.x + rect.w,         (float)rect.y},                 {0,0,0,0}, {0,0}},
        {{(float)rect.x + rect.w,         (float)rect.y + rect.h},        {0,0,0,0}, {0,0}},
        {{(float)rect.x,                  (float)rect.y + rect.h},        {0,0,0,0}, {0,0}}
    };

    float cx = rect.x + rect.w * 0.5f;
    float cy = rect.y + rect.h * 0.5f;

    auto getColor = [&](float px, float py) -> SDL_Color
    {
        if (!grad.enabled)
            return color;

        float t = 0.0f;

        if (grad.isRadial)
        {
            float dx = px - cx;
            float dy = py - cy;
            float dist = std::sqrt(dx*dx + dy*dy);
            t = std::min(1.0f, dist / (std::max(rect.w, rect.h) * 0.5f));
        }
        else
        {
            float angleRad = grad.angle * (M_PI / 180.0f);
            float dx = std::cos(angleRad);
            float dy = std::sin(angleRad);
            float proj = (px - cx) * dx + (py - cy) * dy;
            float maxProj = (rect.w * std::abs(dx) + rect.h * std::abs(dy)) * 0.5f;
            if (maxProj > 0.0f)
                t = std::clamp((proj + maxProj) / (2.0f * maxProj), 0.0f, 1.0f);
        }

        SDL_Color base = color;
        SDL_Color gc = LerpColor(grad.startColor, grad.endColor, t);

        return {
            (Uint8)((gc.r * base.r) / 255),
            (Uint8)((gc.g * base.g) / 255),
            (Uint8)((gc.b * base.b) / 255),
            (Uint8)((gc.a * base.a) / 255)
        };
    };

    vertices[0].color = getColor((float)rect.x, (float)rect.y);
    vertices[1].color = getColor((float)rect.x + rect.w, (float)rect.y);
    vertices[2].color = getColor((float)rect.x + rect.w, (float)rect.y + rect.h);
    vertices[3].color = getColor((float)rect.x, (float)rect.y + rect.h);

    int indices[] = { 0, 1, 2, 0, 2, 3 };

    SDL_RenderGeometry(renderer, nullptr, vertices.data(), 4, indices, 6);
}