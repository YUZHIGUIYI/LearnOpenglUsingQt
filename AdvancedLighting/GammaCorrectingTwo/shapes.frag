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
uniform bool gamma;

out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

vec3 BlinnPhong(vec3 normal, vec3 fragPos, vec3 lightPos, vec3 lightColor) {
    // diffuse
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // specular
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;
    // simple attenuation
    float max_distance = 1.5;
    float distance = length(lightPos - fragPos);
    float attenuation = 1.0 / (gamma ? distance * distance : distance);
    diffuse *= attenuation;
    specular *= attenuation;
    return diffuse + specular;
}

void main() {
    vec3 diffuseTexColor = vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 ambient = light.ambient;
    //........
    //if(gamma) diffuseTexColor = pow(diffuseTexColor, vec3(1.0/2.2));
    vec3 norm = normalize(Normal);

    vec3 result;
    for(int i = -2; i < 2; ++i){
        vec3 lightColor = (2-i) * vec3(0.25);
        result += BlinnPhong(norm,FragPos,light.pos+i*vec3(2,0.0,0.0),lightColor);
    }
    if(gamma)
        ambient = pow(ambient, vec3(2.2));
    result += ambient;
    result *= diffuseTexColor*result;
    if(gamma) result = pow(result, vec3(1.0/2.2));
    if(gl_FrontFacing == false)
        FragColor = vec4(result, 1.0);
}
