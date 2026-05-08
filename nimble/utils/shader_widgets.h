#pragma once

// ============================================================================
// SHADER-INSPIRED WIDGETS
// ============================================================================
// This file provides widget implementations that use the concepts from the
// GLSL shaders in nimble/shaders/, adapted to work with SDL_Renderer.
//
// IMPORTANT: This file must be included AFTER all widget types are defined
// (after widget.h and helpers.h/cpp are included in nimble.cpp)
//
// Shaders available:
//   - rounded_box.frag -> SDF-based rounded box rendering
//   - box_shadow.frag  -> SDF-based box shadow rendering  
//   - blur.frag        -> Distortion-based blur effect
// ============================================================================

#include <SDL3/SDL.h>
#include <cmath>
#include <algorithm>
#include <vector>
#include <string>

// Include the necessary types - these are already included via nimble.cpp
// but we include them here for standalone compilation
#include "../utils/math.h"
#include "../utils/helpers.h"
#include "../widget.h"

// Forward declare functions that are defined in helpers.c and widgets
extern void FillRoundedBoxAA(SDL_Renderer* renderer, SDL_Rect rect, int radius, SDL_Color color, Gradient grad);
extern void DrawRoundedBoxOutlineAA(SDL_Renderer* renderer, SDL_Rect rect, int radius, int borderWidth, SDL_Color color);
extern SDL_Texture* GetShadowTexture(SDL_Renderer* renderer, int w, int h, int radius, int spread, int blur);

// Forward declare BackdropBlur widget
namespace Widgets {
    inline Widget BackdropBlur(int blurRadius, Widget child);
}

// ============================================================================
// SDF Utility Functions (matching the GLSL shaders)
// ============================================================================

namespace SDF {
    // Simple 2D vector for SDF calculations
    struct vec2 {
        float x, y;
        vec2() : x(0), y(0) {}
        vec2(float _x, float _y) : x(_x), y(_y) {}
    };
    
    inline vec2 vec2_abs(const vec2& v) { return vec2(std::abs(v.x), std::abs(v.y)); }
    inline vec2 vec2_max(const vec2& a, const vec2& b) { return vec2(std::max(a.x, b.x), std::max(a.y, b.y)); }
    inline float vec2_min(float a, float b) { return std::min(a, b); }
    inline float vec2_max_f(float a, float b) { return std::max(a, b); }
    inline float vec2_length(const vec2& v) { return std::sqrt(v.x * v.x + v.y * v.y); }
    
    // Rounded box SDF from box_shadow.frag
    // q = abs(p) - b + r
    // return length(max(q,0)) + min(max(q.x,q.y),0) - r
    inline float RoundedBoxSDF(float px, float py, float bx, float by, float r) {
        float qx = std::abs(px) - bx + r;
        float qy = std::abs(py) - by + r;
        float qmax = std::max(qx, 0.0f);
        float qmax2 = std::max(qy, 0.0f);
        float len = std::sqrt(qmax * qmax + qmax2 * qmax2);
        float inner = std::min(std::max(qx, qy), 0.0f);
        return len + inner - r;
    }
    
    // Triangle SDF from rounded_box.frag
    inline float TriangleSDF(float nx, float ny) {
        return vec2_max_f(std::abs(nx) * 0.8660254f + ny * 0.5f, -ny);
    }
}

// ============================================================================
// Shader-Based Widget Implementations
// These must be defined in the Widgets namespace to match existing patterns
// ============================================================================

namespace Widgets {

    // ========================================================================
    // RoundedBoxShader Widget - Uses SDF-based rendering for perfect circles
    // Matches the behavior of the existing RoundedBox but references shader concepts
    // ========================================================================
    inline Widget RoundedBoxShader(Vector2 size, WidgetStyle style, 
                                    Widget child = {}, Vector2 align = {0, 0},
                                    std::string id = "RoundedBoxShader") {
        const bool shouldAutoSize = (size.x <= 0.0f || size.y <= 0.0f);

        Widget w;
        w.id = id;
        w.size = size;
        w.style = style;
        w.debug = {};
        
        w.paint = [style, shouldAutoSize, align]
                    (SDL_Renderer* renderer, SDL_Rect rect, const WidgetStyle& s, 
                     const InputState& input, const std::vector<Widget>& childrenList, 
                     WidgetDebug dbg) {
            
            if (rect.w <= 0 || rect.h <= 0) return;
            
            // 1. Shadow (using SDF-based shadow generation from box_shadow.frag concept)
            if (s.shadowColor.a > 0) {
                SDL_Texture* shadowTex = GetShadowTexture(
                    renderer, rect.w, rect.h,
                    s.radius, s.shadowSpread, s.shadowBlur);
                
                if (shadowTex) {
                    SDL_SetTextureBlendMode(shadowTex, SDL_BLENDMODE_BLEND);
                    SDL_SetTextureColorMod(shadowTex, s.shadowColor.r, s.shadowColor.g, s.shadowColor.b);
                    SDL_SetTextureAlphaMod(shadowTex, s.shadowColor.a);
                    int pad = s.shadowBlur + s.shadowSpread;
                    SDL_FRect shadowRect = {
                        (float)(rect.x + s.shadowOffset.x - pad),
                        (float)(rect.y + s.shadowOffset.y - pad),
                        (float)(rect.w + pad * 2),
                        (float)(rect.h + pad * 2)
                    };
                    SDL_RenderTexture(renderer, shadowTex, nullptr, &shadowRect);
                }
            }

            // 2. Background (SDF-based rounded box - uses FillRoundedBoxAA which is SDF-like)
            FillRoundedBoxAA(renderer, rect, s.radius, s.color, s.gradient);

            // 3. Border (drawn outside of rect)
            if (s.borderWidth > 0 && s.borderColor.a > 0) {
                SDL_Rect currentClip;
                bool hasClip = SDL_RenderClipEnabled(renderer);
                if (hasClip) {
                    SDL_GetRenderClipRect(renderer, &currentClip);
                    SDL_SetRenderClipRect(renderer, nullptr);
                }

                DrawRoundedBoxOutlineAA(renderer, rect, s.radius, s.borderWidth, s.borderColor);

                if (hasClip) {
                    SDL_SetRenderClipRect(renderer, &currentClip);
                }
            }

            // 4. Render child (aligned)
            if (!childrenList.empty()) {
                const auto& c = childrenList[0];
                int cw = c.size.x;
                int ch = c.size.y;

                if (c.expandX > 0.0f)
                    cw = (int)(rect.w * c.expandX);
                else if (cw > rect.w)
                    cw = rect.w;

                if (c.expandY > 0.0f)
                    ch = (int)(rect.h * c.expandY);
                else if (ch > rect.h)
                    ch = rect.h;

                SDL_Rect cRect = {
                    rect.x + (int)((rect.w - cw) / 2.0f + align.x * (rect.w - cw) / 2.0f),
                    rect.y + (int)((rect.h - ch) / 2.0f - align.y * (rect.h - ch) / 2.0f),
                    cw, ch
                };

                c.render(renderer, cRect, input, dbg);
            }
        };

        if (child.paint)
            w.children.push_back(std::move(child));

        if (shouldAutoSize && !w.children.empty()) {
            const Widget& childWidget = w.children[0];
            Vector2 padding = style.padding;
            float border = style.borderWidth * 2.0f;
            w.size.x = childWidget.size.x + padding.x + border;
            w.size.y = childWidget.size.y + padding.y + border;
        }

        return w;
    }

    // ========================================================================
    // BoxShadow Widget - Dedicated shadow-only widget using SDF
    // Can be used standalone or combined with other widgets
    // ========================================================================
    inline Widget BoxShadow(Vector2 size, WidgetStyle style, 
                            Widget child = {}, std::string id = "BoxShadow") {
        const bool shouldAutoSize = (size.x <= 0.0f || size.y <= 0.0f);

        Widget w;
        w.id = id;
        w.size = size;
        w.style = style;
        w.debug = {};
        
        w.paint = [style, shouldAutoSize]
                    (SDL_Renderer* renderer, SDL_Rect rect, const WidgetStyle& s,
                     const InputState& input, const std::vector<Widget>& childrenList,
                     WidgetDebug dbg) {
            
            if (rect.w <= 0 || rect.h <= 0) return;
            
            // Only render shadow if configured
            if (s.shadowColor.a > 0) {
                SDL_Texture* shadowTex = GetShadowTexture(
                    renderer, rect.w, rect.h,
                    s.radius, s.shadowSpread, s.shadowBlur);
                
                if (shadowTex) {
                    SDL_SetTextureBlendMode(shadowTex, SDL_BLENDMODE_BLEND);
                    SDL_SetTextureColorMod(shadowTex, s.shadowColor.r, s.shadowColor.g, s.shadowColor.b);
                    SDL_SetTextureAlphaMod(shadowTex, s.shadowColor.a);
                    int pad = s.shadowBlur + s.shadowSpread;
                    SDL_FRect shadowRect = {
                        (float)(rect.x + s.shadowOffset.x - pad),
                        (float)(rect.y + s.shadowOffset.y - pad),
                        (float)(rect.w + pad * 2),
                        (float)(rect.h + pad * 2)
                    };
                    SDL_RenderTexture(renderer, shadowTex, nullptr, &shadowRect);
                }
            }
            
            // Render child on top
            if (!childrenList.empty()) {
                const auto& c = childrenList[0];
                int cw = c.size.x;
                int ch = c.size.y;

                if (c.expandX > 0.0f)
                    cw = (int)(rect.w * c.expandX);
                else if (cw > rect.w)
                    cw = rect.w;

                if (c.expandY > 0.0f)
                    ch = (int)(rect.h * c.expandY);
                else if (ch > rect.h)
                    ch = rect.h;

                SDL_Rect cRect = {
                    rect.x + (int)((rect.w - cw) / 2.0f),
                    rect.y + (int)((rect.h - ch) / 2.0f),
                    cw, ch
                };

                c.render(renderer, cRect, input, dbg);
            }
        };

        if (child.paint)
            w.children.push_back(std::move(child));

        if (shouldAutoSize && !w.children.empty()) {
            const Widget& childWidget = w.children[0];
            w.size.x = childWidget.size.x;
            w.size.y = childWidget.size.y;
        }

        return w;
    }

    // ========================================================================
    // BlurShader Widget - Uses distortion-based blur from blur.frag
    // Different from Gaussian blur - creates a frosted glass effect
    // Uses the existing Gaussian blur but with a wrapper for the shader concept
    // ========================================================================
    inline Widget BlurShader(int blurRadius, Widget child, std::string id = "BlurShader") {
        // Delegate to existing BackdropBlur which uses GPU-accelerated Gaussian
        // The blur.frag shader uses distortion which is different, but for SDL_Renderer
        // the Gaussian approach is more practical and performant
        return BackdropBlur(blurRadius, child);
    }

    // ========================================================================
    // Glassmorphism Widget - Combines blur + semi-transparent background
    // This is a practical combination of the shader concepts
    // ========================================================================
    inline Widget Glassmorphism(Vector2 size, WidgetStyle style, 
                                 int blurRadius = 10, Widget child = {},
                                 std::string id = "Glassmorphism") {
        // Create a semi-transparent background widget
        WidgetStyle bgStyle = style;
        bgStyle.color.a = (Uint8)(bgStyle.color.a * 0.5f); // More transparent for glass effect
        
        Widget bg;
        bg.size = size;
        bg.style = bgStyle;
        bg.debug = {};
        bg.paint = [bgStyle](SDL_Renderer* renderer, SDL_Rect rect, const WidgetStyle&,
                             const InputState&, const std::vector<Widget>&, WidgetDebug) {
            if (bgStyle.color.a > 0) {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, bgStyle.color.r, bgStyle.color.g, 
                                      bgStyle.color.b, bgStyle.color.a);
                SDL_RenderFillRect(renderer, &rect);
            }
        };

        if (child.paint)
            bg.children.push_back(std::move(child));

        // Apply backdrop blur
        Widget blurred = BackdropBlur(blurRadius, bg);
        
        // Wrap in rounded box for the glass effect
        return RoundedBoxShader(size, style, blurred, {0, 0}, id);
    }

} // namespace Widgets