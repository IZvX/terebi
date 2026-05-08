#version 120

varying vec2 v_texCoord;
uniform sampler2D u_texture;
uniform vec2 u_direction;
uniform float u_radius;
uniform vec2 u_resolution;

void main() {
    vec2 uv = v_texCoord;

    // sample a tiny offset field from the texture itself
    vec2 noise = texture2D(u_texture, uv).rg;

    // convert to signed distortion
    vec2 distortion = (noise - 0.5) * 2.0;

    // scale distortion (u_radius becomes "glass strength")
    distortion *= (u_radius / u_resolution) * 5.0;

    // directional bias (optional, keeps your uniform relevant)
    distortion *= u_direction + vec2(0.001);

    vec4 color = texture2D(u_texture, uv + distortion);

    gl_FragColor = color;
}