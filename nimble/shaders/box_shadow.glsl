// ============================================================================
// Box Shadow Shader - OpenGL Version
// ============================================================================
// SDF-based box shadow rendering for OpenGL
// Compatible with OpenGL 2.1+ (GLSL 120)
// ============================================================================

#version 120

varying vec2 v_uv;
varying vec4 v_color;

uniform vec2 u_size;
uniform float u_radius;
uniform float u_spread;
uniform float u_blur;

float roundedBoxSDF(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + vec2(r);
    return length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - r;
}

void main() {
    vec2 centered = (v_uv - 0.5) * u_size;
    vec2 halfSize = 0.5 * u_size + vec2(u_spread);
    float sdf = roundedBoxSDF(centered, halfSize, u_radius + u_spread);
    float sigma = max(u_blur * 0.5, 1.0);
    float alpha = exp(-(sdf * sdf) / (2.0 * sigma * sigma));

    gl_FragColor = vec4(v_color.rgb, v_color.a * alpha);
}