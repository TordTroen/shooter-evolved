#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vNormal;
out vec2 vTexCoord;
out vec3 vColor;

void main() {
    // Normal matrix handles non-uniform scaling
    vNormal   = mat3(transpose(inverse(model))) * aNormal;
    vTexCoord = aTexCoord;
    vColor    = aColor;
    gl_Position = projection * view * model * vec4(aPosition, 1.0);
}
