#version 450 core

in vec3 vNormal;
in vec2 vTexCoord;
in vec3 vColor;

out vec4 fragColor;

uniform vec3      uColor;
uniform sampler2D uBaseColor;
uniform bool      uHasBaseColor;
// Emissive: skip lighting and use the texture's alpha channel. Caller is
// expected to enable blending (e.g. additive) for transparent effects.
uniform bool      uEmissive;

void main() {
    if (uEmissive) {
        vec4 tex = uHasBaseColor ? texture(uBaseColor, vTexCoord) : vec4(1.0);
        fragColor = vec4(uColor * vColor * tex.rgb, tex.a);
        return;
    }

    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff    = max(dot(normalize(vNormal), lightDir), 0.0);

    vec3 albedo = uColor * vColor;
    if (uHasBaseColor) {
        albedo *= texture(uBaseColor, vTexCoord).rgb;
    }

    vec3 color = albedo * (0.2 + 0.8 * diff);
    fragColor  = vec4(color, 1.0);
}
