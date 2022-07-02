#pragma once
#ifndef MESH_H
#define MESH_H
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions_4_5_Core>
#include <vector>
#include <string>
using namespace std;

struct Vertex {
    QVector3D Position;
    QVector3D Normal;
    QVector2D TexCoords;
};

struct Texture {
    unsigned int id;
    string type;
    string path;
};

class Mesh
{
public:
    // mesh data
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;

    void Draw(QOpenGLShaderProgram &shader);
    Mesh(QOpenGLFunctions_4_5_Core *glFuns,
         vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures);
private:
    // render data
    unsigned int VAO, VBO, EBO;
    void setupMesh();
private:
    QOpenGLFunctions_4_5_Core *m_glFuns;
};

#endif // MESH_H
