#version 450

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_color;
layout(location = 0) out vec4 out_color;

layout(push_constant) uniform RoundedParams {
    vec2 size;
    float radius;
    float feather;
} ubo;

void main() {
    vec2 p = v_uv * ubo.size;
    vec2 center = 0.5 * ubo.size;

    // Normalize position around center
    vec2 n = (p - center) / center;

    // Triangle mask (equilateral-ish, centered, pointing up)
    float tri = max(
        abs(n.x) * 0.8660254 + n.y * 0.5,
        -n.y
    );

    // inside = 1, outside = 0
    float alpha = 1.0 - smoothstep(0.0, max(ubo.feather, 0.001), tri);

    out_color = vec4(v_color.rgb, v_color.a * alpha);
}