#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aOffset;

out vec3 Color;

void main() {
    vec3 pos = aPos * (gl_InstanceID / 100.0);
    gl_Position = vec4(pos, 1.0) + vec4(aOffset, 0.0, 0.0);
    Color = aNormal;
}
