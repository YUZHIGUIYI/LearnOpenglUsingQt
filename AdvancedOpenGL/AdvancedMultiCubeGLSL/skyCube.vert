#version 450 core
layout(location = 0) in vec3 aPos;
layout (std140) uniform Matrices {
    mat4 projection;
    mat4 view;
};

out vec3 TexCoords;

uniform mat4 skyView;
void main() {
   TexCoords = aPos;
   vec4 pos = projection * skyView * vec4(aPos, 1.0f);
   gl_Position = pos.xyww;
   // （z/w）为片段的深度值，将z改为w，
   // w/w=1.0f，天空盒子所有片段深度为1.0f
}
