#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out VS_OUT { vec2 texCoords; } vs_out;
out VS_OUTF { vec3 fragPos; } vs_outf;
out VS_OUTN { vec3 normal; } vs_outn;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vs_out.texCoords = aTexCoords;
    TexCoords = aTexCoords;
    gl_Position = projection * view * model * vec4(aPos.x, aPos.y, aPos.z, 1.0f);
    Normal = mat3(transpose(inverse(model))) * aNormal;
    FragPos = vec3(model * vec4(aPos, 1.0));
    vs_outf.fragPos = FragPos;
    vs_outn.normal = Normal;
}
