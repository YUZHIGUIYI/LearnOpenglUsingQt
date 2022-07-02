#ifndef MODEL_H
#define MODEL_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <QOpenGLTexture>
#include <string.h>
#include "mesh.h"

class Model
{
public:
    // 希望获取模型的范围，之后在processMesh中设置
    float m_maxX{-100.0}, m_minX{100.0};
    float m_maxY{-100.0}, m_minY{100.0};

    vector<Texture> textures_loaded;
    Model(QOpenGLFunctions_4_5_Core *glFuns, const char *path)
        : m_glFuns(glFuns)
    {
        loadModel(path);
    }
    void Draw(QOpenGLShaderProgram &shader) {
        for (unsigned int i = 0; i < meshes.size(); i++) {
            meshes[i].Draw(shader);
        }
    }
private:
    QOpenGLFunctions_4_5_Core *m_glFuns;
    // model data
    vector<Mesh> meshes;
    string directory;
    void loadModel(string path);
    void processNode(aiNode *node, const aiScene *scene);
    Mesh processMesh(aiMesh *mesh, const aiScene *scene);
    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName);
    unsigned int TextureFromFile(const char *path, const string &direction);
};

#endif // MODEL_H
