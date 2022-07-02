#version 450 core
out vec4 FragColor;
in vec3 Normal;
in vec3 Position;
uniform vec3 viewPos;
uniform samplerCube skybox;
void main() {
    float ratio = 1.0 / 2.42;
    vec3 I = normalize(Position - viewPos);
    vec3 R = refract(I, normalize(Normal), ratio);
    FragColor = vec4(texture(skybox, R).rgb, 1.0);
}

