#version 450 core
struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    sampler2D texture_emisson1;
    float shininess;
};
uniform Material material;
// 聚光
struct SpotLight {
    vec3 direction;
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;
};
uniform SpotLight spotLight;

// 定向光
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform DirLight dirLight;

// 点光源
struct PointLight {
    vec3 position;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
#define NR_POINT_LIGHTS 4
uniform PointLight pointLights[NR_POINT_LIGHTS];

out vec4 FragColor;
in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

uniform vec3 lightPos;
uniform vec3 viewPos;  // 摄像机位置

// ambient
vec3 diffuseTexColor = vec3(texture(material.texture_diffuse1, TexCoords));
vec3 specularTexColor = vec3(texture(material.texture_specular1, TexCoords));
vec3 emissonTexColor = texture(material.texture_emisson1, TexCoords).rgb;

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 viewDir);
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main() {
    vec4 texColor = texture(material.texture_diffuse1, TexCoords);
    if (texColor.a < 0.1) discard;  // 透明度小于0.1，丢弃
    FragColor = texColor;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(spotLight.position - FragPos);
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    // clamp函数，它把第一个参数约束在了0.0到1.0之间
    float intensity = clamp((theta - light.outerCutOff)/epsilon, 0.0, 1.0);
    // ambient
    vec3 ambient = diffuseTexColor * light.ambient;
    // diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * diffuseTexColor  * light.diffuse;
    // specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = spec * light.specular * specularTexColor;
    // attenuation
    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance
                               + light.quadratic * (distance * distance));
    //
    diffuse *= attenuation;
    specular *= attenuation;
    // smooth
    diffuse *= intensity;
    specular *= intensity;

    vec3 result = (ambient + diffuse + specular);
    return result;
}
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // combine results
    vec3 ambient = light.ambient * diffuseTexColor;
    vec3 diffuse = light.diffuse * diff * diffuseTexColor;
    vec3 specular = light.specular * spec * specularTexColor;
    return (ambient + diffuse + specular);
}
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0/(light.constant + light.linear * distance +
                             light.quadratic * (distance * distance));
    // combine results
    vec3 ambient = light.ambient * diffuseTexColor;
    vec3 diffuse = light.diffuse * diff * diffuseTexColor;
    vec3 specular = light.specular * spec * specularTexColor;

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return (ambient + diffuse + specular);
}
