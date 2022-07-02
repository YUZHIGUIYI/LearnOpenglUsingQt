#include "model.h"

void Model::loadModel(string path)
{
    Assimp::Importer import;
    const aiScene *scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
    // 空 scene没加载完成 没有根节点 -> 报错，返回
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        qDebug() << "EROOR::ASSIMP::" << import.GetErrorString();
        return;
    }
    directory = path.substr(0, path.find_last_of('/'));  // 需要目录
    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode *node, const aiScene *scene)
{
    // 这其实是一个递归函数
    // Assimp里的结构：每个节点包含一组网格索引，每个索引指向场景对象中的特定网格
    // process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }
    // then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene)
{
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        // 之后在nopenglwidget.cpp loadModel()中继续判断
        if (m_maxX < mesh->mVertices[i].x) m_maxX = mesh->mVertices[i].x;
        if (m_maxY < mesh->mVertices[i].y) m_maxY = mesh->mVertices[i].y;

        if (m_minX > mesh->mVertices[i].x) m_minX = mesh->mVertices[i].x;
        if (m_minY > mesh->mVertices[i].y) m_minY = mesh->mVertices[i].y;
        Vertex vertex;
        // 处理顶点位置、法线和纹理坐标
        QVector3D vector;
        vector.setX(mesh->mVertices[i].x);
        vector.setY(mesh->mVertices[i].y);
        vector.setZ(mesh->mVertices[i].z);

        vertex.Position = vector;

        vector.setX(mesh->mNormals[i].x);
        vector.setY(mesh->mNormals[i].y);
        vector.setZ(mesh->mNormals[i].z);

        vertex.Normal = vector;

        // 有纹理坐标?
        if (mesh->mTextureCoords[0]) {
            QVector2D vec;
            vec.setX(mesh->mTextureCoords[0][i].x);
            vec.setY(mesh->mTextureCoords[0][i].y);
            vertex.TexCoords = vec;
        } else {
            vertex.TexCoords = QVector2D(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }
    // 处理索引
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // 处理材质
    // 如果有纹理
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        vector<Texture> diffuseMaps =
                loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        vector<Texture> specularMaps =
                loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    }
    return Mesh(m_glFuns, vertices, indices, textures);
}

vector<Texture> Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
{
    vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        // 加入skip变量，考虑名单textures_loaded，避免重复加载
        // 最终只加载了两个纹理，而不加载76个纹理
        bool skip = false;
        for (unsigned int j = 0; j < textures_loaded.size(); j++) {
            // 直接，将名单遍历一遍，与这个新提取出来的名字比较，若为0，则表示不是第一次进来，直接加入到textures，不用再加载
            if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
                textures.push_back(textures_loaded[j]);
                skip = true;
                break;
            }
        }
        if (!skip) {
            // 没有在名单中找到，需要加载
            Texture texture;
            texture.id = TextureFromFile(str.C_Str(), directory);  // 文件名称和文件路径
            texture.type = typeName;
            texture.path = str.C_Str();  // 类型转换
            textures.push_back(texture);
            textures_loaded.push_back(texture);  // 加入名单
        }
    }
    return textures;
}

unsigned int Model::TextureFromFile(const char *path, const string &directory)
{
    string filename = string(path);
    filename = directory + '/' + filename;

    QOpenGLTexture *texture = new QOpenGLTexture(QImage(filename.c_str()).mirrored());  // 倒置，正变负，负变正
    if (texture == NULL) qDebug() << "texture is NULL";
    else qDebug() << filename.c_str() << "loaded";

    return texture->textureId();
}
