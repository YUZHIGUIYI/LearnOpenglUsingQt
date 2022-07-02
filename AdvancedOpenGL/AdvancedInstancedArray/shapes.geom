#version 450 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;
in VS_OUT { vec2 texCoords; } gs_in[];
in VS_OUTF { vec3 fragPos; } vs_outf[];
in VS_OUTN { vec3 normal; } vs_outn[];

out vec2 TexCoords;
out vec3 FragPos;
out vec3 Normal;

uniform float time;

vec3 GetNormal() {
    vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
    vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
    return normalize(cross(a, b));
}

vec4 explode(vec4 position, vec3 normal) {
    float magnitude = 3.0;
    vec3 direction = normal * ((sin(time) + 1.0) / 2.0) * magnitude;
    return position + vec4(direction, 0.0);
}

void main() {
    vec3 normal = GetNormal();
    gl_Position = explode(gl_in[0].gl_Position, normal);
    TexCoords = gs_in[0].texCoords;
    FragPos = vs_outf[0].fragPos;
    Normal = vs_outn[0].normal;
    EmitVertex();
    gl_Position = explode(gl_in[1].gl_Position, normal);
    TexCoords = gs_in[1].texCoords;
    FragPos = vs_outf[1].fragPos;
    Normal = vs_outn[1].normal;
    EmitVertex();
     gl_Position = explode(gl_in[2].gl_Position, normal);
    TexCoords = gs_in[2].texCoords;
    FragPos = vs_outf[2].fragPos;
    Normal = vs_outn[2].normal;
    EmitVertex();
    EndPrimitive();
}
