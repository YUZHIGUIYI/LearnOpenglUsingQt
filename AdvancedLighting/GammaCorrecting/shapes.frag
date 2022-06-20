#version 450 core
#extension GL_NV_shadow_samplers_cube : enable
struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    sampler2D texture_reflection1;
    float shininess;
};

struct Light {
    vec3 pos;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform Light light;


uniform Material material;
uniform vec3 viewPos;
uniform samplerCube skybox;

out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

void main() {
    vec3 diffuseTexColor=vec3(texture2D(material.texture_diffuse1, TexCoords));
    // 人为下拱
//    float gamma = 2.2;
//    diffuseTexColor = pow(diffuseTexColor, vec3(gamma));

    // 在进行一些列计算之后（在线性空间中），人为上供，结合Qt的代码，实现gamma校正
    float gamma = 1.0/2.2;
    diffuseTexColor = pow(diffuseTexColor, vec3(gamma));


    // ambient
    vec3 ambient = diffuseTexColor*light.ambient;
    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.pos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff *diffuseTexColor*light.diffuse;
    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 16.0);
    vec3 specular =  spec * 0.6 * light.specular;
    //reflection

    vec3 result = (ambient + diffuse + specular);
    FragColor = vec4(result, 1.0);
}
