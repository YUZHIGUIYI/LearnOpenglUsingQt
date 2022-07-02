#ifndef VERTICES_H
#define VERTICES_H
float planeVertices[] = {
    // positions          // texture Coords (note we set these higher than 1 (together with GL_REPEAT as texture wrapping mode). this will cause the floor texture to repeat)
     25.0f, 0.0f,  25.0f,  1.0f, 0.0f,
    -25.0f, 0.0f, -25.0f,  0.0f, 1.0f,
    -25.0f, 0.0f,  25.0f,  0.0f, 0.0f,

     25.0f, 0.0f,  25.0f,  1.0f, 0.0f,
     25.0f, 0.0f, -25.0f,  1.0f, 1.0f,
    -25.0f, 0.0f, -25.0f,  0.0f, 1.0f
};
float transparentVertices[] = {
    // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
    0.0f, -2.5f,  0.0f,  0.0f,  0.0f,
    0.0f,  2.5f,  0.0f,  0.0f,  1.0f,
    6.66f,  2.5f,  0.0f,  1.0f,  1.0f,

    0.0f, -2.5f,  0.0f,  0.0f,  0.0f,
    6.66f,  2.5f,  0.0f,  1.0f,  1.0f,
    6.66f, -2.5f,  0.0f,  1.0f,  0.0f
};
float quadVertices[] = {
    // positions  // colors
    -0.05f, 0.05f, 1.0f, 0.0f, 0.0f,
    0.05f, -0.05f, 0.0f, 1.0f, 0.0f,
    -0.05f, -0.05f, 0.0f, 0.0f, 1.0f,
    -0.05f, 0.05f, 1.0f, 0.0f, 0.0f,
    0.05f, -0.05f, 0.0f, 1.0f, 0.0f,
    0.05f, 0.05f, 0.0f, 1.0f, 1.0f
};
// 其顶点坐标就是纹理坐标
float skyboxVertices[] = {
    // positions
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,
    1.0f,  1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,
    1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
    1.0f,  1.0f, -1.0f,
    1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
    1.0f, -1.0f,  1.0f
};
#endif // VERTICES_H
