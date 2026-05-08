#version 450

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_color;
layout(location = 0) out vec4 out_color;

layout(push_constant) uniform ShadowParams {
    vec2 size;
    float radius;
    float spread;
    float blur;
} ubo;

float roundedBoxSDF(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + vec2(r);
    return length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - r;
}

void main() {
    vec2 centered = (v_uv - 0.5) * ubo.size;
    vec2 halfSize = 0.5 * ubo.size + vec2(ubo.spread);
    float sdf = roundedBoxSDF(centered, halfSize, ubo.radius + ubo.spread);
    float sigma = max(ubo.blur * 0.5, 1.0);
    float alpha = exp(-(sdf * sdf) / (2.0 * sigma * sigma));
    out_color = vec4(v_color.rgb, v_color.a * alpha);
}
