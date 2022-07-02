#version 450 core
#extension GL_NV_shadow_samplers_cube : enable
struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    sampler2D texture_reflection1;
    float shininess;
};

struct Light {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform Light light;


uniform Material material;
uniform vec3 viewPos;
uniform samplerCube skybox;

out vec4 FragColor;

in vec3 Color;

void main() {

    FragColor = vec4(Color, 1.0);
}
