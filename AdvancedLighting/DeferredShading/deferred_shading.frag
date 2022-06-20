#version 450 core
out vec4 FragColor;

//in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

struct Light {
    vec3 Position;
    vec3 Color;

    float Linear;
    float Quadratic;
    float Radius;
};

uniform Light lights;
uniform vec3 viewPos;

void main()
{
//     vec2 tex_offset = 1.0 / textureSize(gPosition, 0);
    // retrieve data from gbuffer
    vec3 FragPos = texture(gPosition, gl_FragCoord.xy/textureSize(gPosition, 0)).rgb;
    vec3 Normal = texture(gNormal, gl_FragCoord.xy/textureSize(gNormal, 0)).rgb;
    vec3 Diffuse = texture(gAlbedoSpec, gl_FragCoord.xy/textureSize(gAlbedoSpec, 0)).rgb;
    float Specular = texture(gAlbedoSpec, gl_FragCoord.xy/textureSize(gAlbedoSpec, 0)).a;

    // then calculate lighting as usual
    vec3 lighting  = Diffuse * 0.2; // hard-coded ambient component
    vec3 viewDir  = normalize(viewPos - FragPos);

    // calculate distance between light source and current fragment
    float distance = length(lights.Position - FragPos);

    // diffuse
    vec3 lightDir = normalize(lights.Position - FragPos);
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * lights.Color;
    // specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
    vec3 specular = lights.Color * spec * Specular;
    // attenuation
    float attenuation = 1.0 / (1.0 + lights.Linear * distance + lights.Quadratic * distance * distance);
    diffuse *= attenuation;
    specular *= attenuation;
    lighting += diffuse + specular;

    FragColor = vec4(lighting, 1.0);
}
