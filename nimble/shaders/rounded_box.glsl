// ============================================================================
// Rounded Box Shader - OpenGL Version
// ============================================================================
// SDF-based rounded box rendering for OpenGL
// Compatible with OpenGL 2.1+ (GLSL 120)
// ============================================================================

#version 120

varying vec2 v_uv;
varying vec4 v_color;

uniform vec2 u_size;
uniform float u_radius;
uniform float u_feather;

void main() {
    vec2 p = v_uv * u_size;
    vec2 center = 0.5 * u_size;

    // Normalize position around center
    vec2 n = (p - center) / center;

    // Triangle mask (equilateral-ish, centered, pointing up)
    float tri = max(
        abs(n.x) * 0.8660254 + n.y * 0.5,
        -n.y
    );

    // inside = 1, outside = 0
    float alpha = 1.0 - smoothstep(0.0, max(u_feather, 0.001), tri);

    gl_FragColor = vec4(v_color.rgb, v_color.a * alpha);
}