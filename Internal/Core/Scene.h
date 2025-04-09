#pragma once
#include <vector>
#include <vec2.hpp>
#include <mat4x4.hpp>
#include <glad/glad.h>
#include "Material.h"
#include "Mesh.h"
#include "Texture.h"
#include "Light.h"
#include "Render/RenderResHelper.h"
#include <bvh.h>
#include <bvh_translator.h>

using namespace RadeonRays;
class Renderer;

struct Indice
{
    Indice(int _x, int _y, int _z) : x(_x), y(_y), z(_z) {}
    int x, y, z;
};

class Scene
{
    friend class ModelLoader;

public:
    Scene() 
    {
        sceneBvh = new Bvh(10.0f, 64, true);
    }
    ~Scene()
    {
        for(int i = 0; i < meshes.size(); i++)
        {
            if(meshes[i])
            {
                delete meshes[i];
                meshes[i] = nullptr;
            }
        }

        for(int i = 0; i < textures.size(); i++)
        {
            if(textures[i])
            {
                delete textures[i];
                textures[i] = nullptr;
            }
        }
        if(envMap)
        {
            delete envMap;
        }
    }

    void buildScene();
    int getTopBVHIndex() const {return bvhTranslator.topLevelIndex;}
    bool getSceneDirty() const {return bDirty;}

private:
    void createTLAS();
    void createBLAS();
    void prepareMeshData();
    void prepareTransform();
    void prepareTexture();

    Bvh* sceneBvh;
    BvhTranslator bvhTranslator;

    std::vector<vec3> vertices;
    std::vector<Indice> indices;
    std::vector<vec3> normals;
    std::vector<vec2> uvs;
    std::vector<Material> materials;
    std::vector<Light> lights;

    std::vector<Mesh*> meshes;
    std::vector<MeshInstance> meshInstances;
    std::vector<mat4> transforms;
    std::vector<Texture*> textures;
    std::vector<unsigned char> textureMapsArray;
    Texture* envMap;

    bool bDirty = false;

public:
    GLuint getAccumTexId() const {return accumTex.texId;}
    GLuint getVertTexId() const {return verticeTex.texId;}
    GLuint getIndiceTexId() const {return indicesTex.texId;}
    GLuint getNormalTexId() const {return normalTex.texId;}
    GLuint getUVTexId() const {return uvsTex.texId;}
    GLuint getBVHTexId() const {return bvhTex.texId;}
    GLuint getMatTexId() const {return matTex.texId;}
    GLuint getLightTexId() const {return lightTex.texId;}
    GLuint getEnvTexId() const {return envTex.texId;}
    GLuint getTransformTexId() const {return instanceTransformTex.texId;}
    GLuint getTextureArrayId() const {return textureArrayTex.texId;}

    void InitFBO();
    GLuint pathTracingFBO;
    GLuint pathTracingTexId;
    GLuint accumTexId;

    GLuint toneMappingFBO;
    GLuint toneMappingTexId;

private:
    void UploadDataToGpu();

private:
    GPUTexture accumTex;

    GPUTexture verticeTex;

    GPUTexture indicesTex;

    GPUTexture normalTex;

    GPUTexture uvsTex;

    GPUTexture lightTex;

    GPUTexture matTex;

    GPUTexture instanceTransformTex;

    GPUTexture bvhTex;

    GPUTexture envTex;

    GPUTexture textureArrayTex;
};
