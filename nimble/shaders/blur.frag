// Save as "blur.frag" or embed as string
#version 120
varying vec2 v_texCoord;
uniform sampler2D u_texture;
uniform vec2 u_direction; // (1.0, 0.0) for horizontal, (0.0, 1.0) for vertical
uniform float u_radius;
uniform vec2 u_resolution;

void main() {
    vec2 uv = v_texCoord;
    vec4 color = vec4(0.0);
    float totalWeight = 0.0;
    float sigma = max(u_radius / 2.0, 1.0);
    
    // Standard Gaussian Kernel
    for (float i = -u_radius; i <= u_radius; i++) {
        float weight = exp(-(i * i) / (2.0 * sigma * sigma));
        vec2 offset = (i * u_direction) / u_resolution;
        color += texture2D(u_texture, uv + offset) * weight;
        totalWeight += weight;
    }
    gl_FragColor = color / totalWeight;
}