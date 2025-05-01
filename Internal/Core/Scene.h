#pragma once
#include <vector>
#include <memory>
#include <typeindex>
#include <any>
#include <unordered_map>

#include <vec2.hpp>
#include <mat4x4.hpp>
#include <glad/glad.h>
#include "Material.h"
#include "Mesh.h"

#include "Light.h"
#include "Render/RenderInterface.h"
#include "Render/Texture.h"
#include <bvh.h>
#include <bvh_translator.h>
#include "entt.hpp"

using namespace RadeonRays;
class Renderer;

struct Indice
{
    Indice() {}
    Indice(Uint32 _x, Uint32 _y, Uint32 _z) : x(_x), y(_y), z(_z) {}
    Uint32 x, y, z;
};

struct ShadowMapCache
{
    ShadowMapCache()
    {
    }
    ~ShadowMapCache()
    {
    }
    ShadowMapCache(const ShadowMapCache& cache)
    {
        this->light = cache.light;
        this->shadowPassDepthTexIds = cache.shadowPassDepthTexIds;
        this->bMultiMats = cache.bMultiMats;
        for(Int i = 0; i < 6; i++)
        {
            this->lightMats[i] = cache.lightMats[i];
        }
    }
    ShadowMapCache& operator=(const ShadowMapCache&) = delete;

    glm::mat4 lightMats[6];
    Light* light;
    GLuint shadowPassDepthTexIds;
    Bool bMultiMats;
};

struct EnvMap
{
    EnvMap()
    {
        bCubeMap = false;
    }
    std::vector<Texture> envTextures;
    Bool bCubeMap;
};

class RenderResource
{
public:
    RenderResource() = default;
    RenderResource(const RenderResource &) = default;
    RenderResource(RenderResource &&) noexcept = default;
    ~RenderResource() = default;

    RenderResource &operator=(const RenderResource &) = default;
    RenderResource &operator=(RenderResource &&) noexcept = default;

    template <typename T, typename... Args>
    inline T& add(Args &&...args)
    {
        return storages[typeid(T)].emplace<T>(T{std::forward<Args>(args)...});
    }

    template <typename T>
    inline const T& get() const
    {
        return std::any_cast<const T &>(storages.at(typeid(T)));
    }

    template <typename T> 
    T& get()
    {
        return const_cast<T&>(const_cast<const RenderResource *>(this)->get<T>());
    }

    template <typename T> 
    Bool hasResource() const
    {
        return storages.contains(typeid(T));
    }

private:
    std::unordered_map<std::type_index, std::any> storages;
};

// class MeshInitializer
// {
// public:
//     MeshInitializer(entt::registry& _reg) : reg(_reg)
//     {
//         sceneBvh = std::make_shared<Bvh>(10.0f, 64, true);
//     }

//     ~MeshInitializer()
//     {

//     }

// private:


// private:
//     entt::registry& reg;
//     std::shared_ptr<Bvh> sceneBvh;
//     BvhTranslator bvhTranslator;
// };

class Scene
{
    friend class ModelLoader;

public:
    Scene() 
    {
        sceneBvh = std::make_shared<Bvh>(10.0f, 64, true);
        envMap = std::make_shared<EnvMap>();
        curOutputTex = 1;
    }
    ~Scene()
    {
        CleanScene();
    }

    void BuildScene();
    void CleanScene();

    void InitShadowMapFBO();
    RadeonRays::bbox GetSceneBoundingBox() {return sceneBvh->Bounds();}

    int curOutputTex;

    std::vector<ShadowMapCache> shadowMapCaches;

private:
    void UploadDataToGpu();

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

    const std::vector<Light>& GetSceneLights() const 
    {
        return lights;
    }

public:
    const std::vector<MeshBatch*>& GetMeshBatches() const
    {
        return meshBatches;
    }

private:
    std::vector<Mesh*> meshes;
    std::vector<MeshInstance> meshInstances;
    std::vector<Light> lights;

    void CombineMesh();
    std::vector<MeshBatch*> meshBatches;

private:
    void createTLAS();
    void createBLAS();
    void prepareMeshData();

    std::shared_ptr<Bvh> sceneBvh;
    BvhTranslator bvhTranslator;

    std::vector<vec3> vertices;
    std::vector<Indice> indices;
    std::vector<vec3> normals;
    std::vector<vec2> uvs;

    void BuildEnvMap();
    std::shared_ptr<EnvMap> envMap;

public:
    GLuint getVertTexId() const {return verticeTex.texId;}
    GLuint getIndiceTexId() const {return indicesTex.texId;}
    GLuint getNormalTexId() const {return normalTex.texId;}
    GLuint getUVTexId() const {return uvsTex.texId;}
    GLuint getBVHTexId() const {return bvhTex.texId;}
    GLuint getEnvTexId() const {return envTex.texId;}
    GLuint getIrradianceEnvTexId() const {return irradianceEnvTex.texId;}

private:
    GPUTexture verticeTex;
    GPUTexture indicesTex;
    GPUTexture normalTex;
    GPUTexture uvsTex;
    GPUTexture bvhTex;
    GPUTexture envTex;
    GPUTexture irradianceEnvTex;
};
