#version 450 core
out vec4 FragColor;
in vec2 TexCoords;
struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    float shininess;
};
uniform Material material;

//uniform sampler2D depthMap;
uniform float near_plane;
uniform float far_plane;
// required when using a perspective projection matrix
float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // Back to NDC
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
}
void main() {
    float depthValue = texture(material.texture_diffuse1, TexCoords).r;
    // FragColor = vec4(vec3(LinearizeDepth(depthValue) / far_plane), 1.0); // perspective
    FragColor = vec4(vec3(depthValue), 1.0); // orthographic
}
