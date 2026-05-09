#version 450 core

in vec3 vNormal;
in vec2 vTexCoord;

out vec4 fragColor;

uniform vec3 uColor;

void main() {
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff    = max(dot(normalize(vNormal), lightDir), 0.0);
    vec3  color   = uColor * (0.2 + 0.8 * diff);
    fragColor = vec4(color, 1.0);
}
