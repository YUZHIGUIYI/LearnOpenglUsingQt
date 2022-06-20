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

const float offset = 1.0/300.0;

void main() {
    // 反相
    //FragColor = vec4(vec3(1.0 - texture(material.texture_diffuse1, TexCoords)), 1.0f);
    // 灰度
    vec2 offsets[9] = vec2[] (
            vec2(-offset, offset),
            vec2(0.0, offset),
            vec2(offset, offset),
            vec2(-offset, 0.0),
            vec2(0.0, 0.0),
            vec2(offset, 0.0),
            vec2(-offset, -offset),
            vec2(0.0f, -offset),
            vec2(offset, -offset)
            );
    float kernel[9] =float [](-1,-1,-1,-1,9,-1,-1,-1,-1);
    vec3 sampleTex[9];
    for (int i = 0; i < 9; i++) {
        sampleTex[i] = vec3(texture(material.texture_diffuse1, TexCoords.st + offsets[i]));
    }
    vec3 col = vec3(0.0);
    for (int i = 0; i < 9; i++) col += sampleTex[i] * kernel[i];
    FragColor = vec4(col, 1.0);

    //FragColor = texture(material.texture_diffuse1, TexCoords);
    //float average = 0.2126*FragColor.r + 0.7152*FragColor.g + 0.722*FragColor.b;
    //FragColor = vec4(average, average, average, 1.0f);
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
