#version 450

// ============================================================================
// Blur Shader - Distortion-based blur effect
// ============================================================================
// This shader creates a frosted glass effect by sampling distortion from
// the texture itself. It's different from Gaussian blur - it creates a
// more organic, refractive look.
//
// For Vulkan/SPIR-V compilation, this shader uses:
// - location bindings for all inputs/outputs
// - push constants for uniforms
// - texture2D -> texture (GLSL 4.5+)
// ============================================================================

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D u_texture;

layout(push_constant) uniform BlurParams {
    vec2 u_direction;
    float u_radius;
    vec2 u_resolution;
} ubo;

void main() {
    vec2 uv = v_texCoord;

    // Sample a tiny offset field from the texture itself
    vec2 noise = texture(u_texture, uv).rg;

    // Convert to signed distortion
    vec2 distortion = (noise - 0.5) * 2.0;

    // Scale distortion (u_radius becomes "glass strength")
    distortion *= (ubo.u_radius / ubo.u_resolution) * 5.0;

    // Directional bias (optional, keeps your uniform relevant)
    distortion *= ubo.u_direction + vec2(0.001);

    vec4 color = texture(u_texture, uv + distortion);

    out_color = color;
}