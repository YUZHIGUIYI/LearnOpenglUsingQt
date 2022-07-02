#ifndef VERTICES_H
#define VERTICES_H
float planeVertices[] = {
    // positions          // texture Coords (note we set these higher than 1 (together with GL_REPEAT as texture wrapping mode). this will cause the floor texture to repeat)
     25.0f, 0.0f,  25.0f,  1.0f, 0.0f,
    -25.0f, 0.0f,  25.0f,  0.0f, 0.0f,
    -25.0f, 0.0f, -25.0f,  0.0f, 1.0f,

     25.0f, 0.0f,  25.0f,  1.0f, 0.0f,
    -25.0f, 0.0f, -25.0f,  0.0f, 1.0f,
     25.0f, 0.0f, -25.0f,  1.0f, 1.0f
};
float transparentVertices[] = {
    // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
    0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
    0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
    1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

    0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
    1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
    1.0f,  0.5f,  0.0f,  1.0f,  0.0f
};
#endif // VERTICES_H
