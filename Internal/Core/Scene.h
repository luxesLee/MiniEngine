#pragma once
#include <vector>
#include <memory>
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
    Indice() {}
    Indice(Uint32 _x, Uint32 _y, Uint32 _z) : x(_x), y(_y), z(_z) {}
    Uint32 x, y, z;
};

class Scene
{
    friend class ModelLoader;

public:
    Scene() 
    {
        sceneBvh = std::make_shared<Bvh>(10.0f, 64, true);
        curOutputTex = 1;
    }
    ~Scene()
    {
        CleanScene();
    }

    void BuildScene();
    void CleanScene();

    void InitFBO();
    void DeleteFBO();

    GLuint outputFBO;
    GLuint outputTex[2];
    int curOutputTex;

    GLuint pathTracingFBO;
    GLuint pathTracingTexId;
    GLuint accumTexId;

    GLuint toneMappingFBO;
    GLuint toneMappingTexId;

    GLuint deferredBasePassFBO;
    GLuint GBufferTexId0, GBufferTexId1, GBufferTexId2, GBufferTexId3;
    GLuint basePassDepthStencilTexId;

private:
    void UploadDataToGpu();
    void DeleteGPUData();

public:
    int getTopBVHIndex() const 
    {
        return bvhTranslator.topLevelIndex;
    }
    int getLightNum() const 
    {
        return lights.size();
    }
    
    void AppendLight(Light& light)
    {
        lights.push_back(light);
    }
    void AppendLightMesh(const Light& light);

public:
    const std::vector<MeshBatch*>& GetMeshBatches() const
    {
        return meshBatches;
    }

private:
    std::vector<Mesh*> meshes;
    std::vector<MeshInstance> meshInstances;

    void CombineMesh();
    std::vector<MeshBatch*> meshBatches;

private:
    void createTLAS();
    void createBLAS();
    void prepareMeshData();
    void prepareTransform();
    void prepareTexture();

    std::shared_ptr<Bvh> sceneBvh;
    BvhTranslator bvhTranslator;

    std::vector<vec3> vertices;
    std::vector<Indice> indices;
    std::vector<vec3> normals;
    std::vector<vec2> uvs;
    std::vector<Material> materials;
    std::vector<Light> lights;

    std::vector<mat4> transforms;
    std::vector<Texture*> textures;
    std::vector<Uchar> textureMapsArray;
    std::shared_ptr<Texture> envMap;

public:
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

private:
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
