#include <algorithm>
#include "Scene.h"
#include "Camera.h"
#include "Config.h"
#include "Util/LTCMatrix.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#include "stb_image.h"

void Scene::BuildScene()
{
    if(g_Config->lightMode == LightMode::PathTracing)
    {
        createBLAS();   // Bottom Level Acceleration Structure -- for mesh
        createTLAS();   // Top Level Acceleration Structure -- for meshInstance
        bvhTranslator.Process(sceneBvh.get(), meshes, meshInstances);
        prepareMeshData();
        prepareTexture();
    }
    else
    {
        CombineMesh();
        prepareTexture();
        // 目的是拿包围盒
        createBLAS();
        createTLAS();
    }
    BuildEnvMap();
    UploadDataToGpu();
}

void Scene::CleanScene()
{    
    vertices.resize(0);
    indices.resize(0);
    normals.resize(0);
    uvs.resize(0);
    lights.resize(0);
    for(int i = 0; i < meshes.size(); i++)
    {
        if(meshes[i])
        {
            delete meshes[i];
            meshes[i] = nullptr;
        }
    }
    meshes.resize(0);
    meshInstances.resize(0);
    for(int i = 0; i < textures.size(); i++)
    {
        if(textures[i])
        {
            delete textures[i];
            textures[i] = nullptr;
        }
    }
    textureMapsArray.resize(0);
    envMap = std::make_shared<EnvMap>();
    sceneBvh = std::make_shared<Bvh>(10.0f, 64, true);

    for(int i = 0; i < meshBatches.size(); i++)
    {
        if(meshBatches[i])
        {
            delete meshBatches[i];
            meshBatches[i] = nullptr;
        }
    }
    meshBatches.resize(0);
}

void Scene::AppendLightMesh(const Light &light)
{
    if(light.type == DIRECTIONAL_LIGHT)
    {
        return;
    }

    Material material;
    material.emission = light.color;
    // materials.push_back(material);

    auto AppendMesh = [&](Mesh* meshes, std::vector<glm::vec3>& vertArray, std::vector<glm::vec3>& normalArray)
    {
        meshes->vertices.insert(meshes->vertices.end(), vertArray.begin(), vertArray.end());
        meshes->normals.insert(meshes->normals.end(), normalArray.begin(), normalArray.end());
    };
    Mesh* mesh = new Mesh();
    if(light.type == POINT_LIGHT)
    {
        glm::vec3 position = light.position;
        glm::vec3 normal = glm::vec3(1, 1, 1);

        AppendMesh(mesh, 
            std::vector<glm::vec3>{position, position, position}, 
            std::vector<glm::vec3>{normal, normal, normal, normal, normal, normal});
    }
    else if(light.type == SPOT_LIGHT)
    {

    }
    else if(light.type == QUAD_LIGHT)
    {
        glm::vec3 position = light.position;
        glm::vec3 xAxis = light.u - position, yAxis = light.v - position;
        glm::vec3 normal = glm::normalize(glm::cross(xAxis, yAxis));
        glm::vec3 v1 = position;
        glm::vec3 v2 = position + xAxis;
        glm::vec3 v3 = position + yAxis;
        glm::vec3 v4 = position + xAxis + yAxis;

        AppendMesh(mesh, 
                std::vector<glm::vec3>{v1, v2, v4, v1, v4, v3}, 
                std::vector<glm::vec3>{normal, normal, normal, normal, normal, normal});
    }
    meshes.push_back(mesh);

    // MeshInstance meshInstance(meshes.size() - 1, 
    //                             materials.size() - 1, 
    //                             glm::mat4(1.0f));
    // meshInstances.push_back(meshInstance);
}

void Scene::CombineMesh()
{
    std::sort(meshInstances.begin(), meshInstances.end(), [&](MeshInstance& instance1, MeshInstance& instance2)
    {
        return instance1.meshID != instance2.meshID ? instance1.meshID < instance2.meshID : instance1.materialID < instance2.materialID;
    });

    auto CopyData = [&](MeshBatch* meshBatch, auto& meshes, auto& meshInstances, int index)
    {
        meshBatch->instanceCount++;
        meshBatch->vertices.insert(meshBatch->vertices.end(), 
            meshes[meshInstances[index].meshID]->vertices.begin(), 
            meshes[meshInstances[index].meshID]->vertices.end());
        meshBatch->indices.insert(meshBatch->indices.end(), 
            meshes[meshInstances[index].meshID]->indices.begin(), 
            meshes[meshInstances[index].meshID]->indices.end());
        meshBatch->normals.insert(meshBatch->normals.end(), 
            meshes[meshInstances[index].meshID]->normals.begin(), 
            meshes[meshInstances[index].meshID]->normals.end());
        meshBatch->uvs.insert(meshBatch->uvs.end(), 
            meshes[meshInstances[index].meshID]->uvs.begin(), 
            meshes[meshInstances[index].meshID]->uvs.end());
        for(int i = 0; i < meshes[meshInstances[index].meshID]->vertices.size(); i++)
        {
            meshBatch->materialIDs.push_back(meshInstances[index].materialID);
        }
    };

    MeshBatch* meshBatch = new MeshBatch;
    Int pre = 0;
    CopyData(meshBatch, meshes, meshInstances, pre);
    for(Int i = 1; i < meshInstances.size(); i++)
    {
        if(meshInstances[i].meshID != meshInstances[pre].meshID)
        {
            meshBatches.push_back(meshBatch);
            meshBatch = new MeshBatch;
        }
       CopyData(meshBatch, meshes, meshInstances, i);
    }
    meshBatches.push_back(meshBatch);
}

void Scene::createTLAS()
{
    int n = meshInstances.size();
    std::vector<RadeonRays::bbox> bounds(n);

#pragma omp parallel for
    for(int i = 0; i < n; i++)
    {
        bounds[i] = transformBy(meshInstances[i].transform, meshes[meshInstances[i].meshID]->bvh->Bounds());
    }

    sceneBvh->Build(bounds.data(), n);
}

void Scene::createBLAS()
{
#pragma omp parallel for
    for(int i = 0; i < meshes.size(); ++i)
    {
        meshes[i]->BuildBvh();
    }
}

void Scene::prepareMeshData()
{
    int curVerticeNum = 0;
    for(int i = 0; i < meshes.size(); i++)
    {
        int indiceSize = meshes[i]->bvh->GetNumIndices();
        const int* triIndices = meshes[i]->bvh->GetIndices();
        for(int j = 0; j < indiceSize; j++)
        {
            int index = triIndices[j] * 3;
            indices.push_back(Indice(index + curVerticeNum, index + 1 + curVerticeNum, index + 2 + curVerticeNum));
        }
        vertices.insert(vertices.end(), meshes[i]->vertices.begin(), meshes[i]->vertices.end());
        normals.insert(normals.end(), meshes[i]->normals.begin(), meshes[i]->normals.end());
        uvs.insert(uvs.end(), meshes[i]->uvs.begin(), meshes[i]->uvs.end());
        curVerticeNum += meshes[i]->vertices.size();
    }
}

void Scene::prepareTexture()
{
    int reqWidth = g_Config->texWidth, reqHeight = g_Config->texHeight;
    int texBytes = reqWidth * reqHeight * 4;
    textureMapsArray.resize(texBytes * textures.size());

#pragma omp parallel for
    for(int i = 0; i < textures.size(); i++)
    {
        int texWidth = textures[i]->width;
        int texHeight = textures[i]->height;

        // Resize textures to fit 2D texture array
        if (texWidth != reqWidth || texHeight != reqHeight)
        {
            unsigned char* resizedTex = new unsigned char[texBytes];
            stbir_resize_uint8(&textures[i]->data[0], texWidth, texHeight, 0, resizedTex, reqWidth, reqHeight, 0, 4);
            std::copy(resizedTex, resizedTex + texBytes, &textureMapsArray[i * texBytes]);
            delete[] resizedTex;
        }
        else
            std::copy(textures[i]->data.begin(), textures[i]->data.end(), &textureMapsArray[i * texBytes]);
    }
}

void Scene::BuildEnvMap()
{
    if(envMap->envTextures.size() == 0)
    {
        return;
    }

    if(envMap->bCubeMap)
    {
        envTex = RenderResHelper::generateCubeEnvMap(envMap->envTextures);
        irradianceEnvTex = generateIrradianceMap(envTex);
    }
    else
    {

    }
}

void Scene::InitShadowMapFBO()
{
    if(!g_Config->bShadeShadow)
    {
        return;
    }

    std::sort(lights.begin(), lights.end(), [&](Light& light1, Light& light2)
    {
        return light1.type < light2.type;
    });

    shadowMapCaches.resize(lights.size());
    for(Int i = 0; i < lights.size(); i++)
    {
        ShadowMapCache& cache = shadowMapCaches[i];
        cache.light = &lights[i];
        if(lights[i].type == LightType::DIRECTIONAL_LIGHT && g_Config->bCascadeShadow)
        {
            cache.bMultiMats = true;
 
            TextureDesc shadowDepthTexDesc{g_Config->shadowDepthWidth, g_Config->shadowDepthHeight, g_Config->cascadeLevel + 1, TextureType::TEXTURE_2D_ARRAY, TextureFormat::DEPTH_COMPONENT32F,
                                        NEAREST_CLAMP_TO_EDGE_WHITE_BORDER_SAMPLER, 0, nullptr, DataFormat::DataFormat_DEPTH_COMPONENT, DataType::FLOAT};
            cache.shadowPassDepthTexIds = (generateTexture(shadowDepthTexDesc)).texId;
        }
        else if(lights[i].type == LightType::POINT_LIGHT)
        {
            cache.bMultiMats = true;
            TextureDesc shadowDepthTexDesc{g_Config->shadowDepthWidth, g_Config->shadowDepthHeight, 0, TextureType::TEXTURE_CUBE_MAP, TextureFormat::DEPTH_COMPONENT32F,
                                        NEAREST_CLAMP_TO_EDGE_WHITE_BORDER_SAMPLER, 0, nullptr, DataFormat::DataFormat_DEPTH_COMPONENT,  DataType::FLOAT};
            cache.shadowPassDepthTexIds = (generateTexture(shadowDepthTexDesc)).texId;
        }
        else
        {
            cache.bMultiMats = false;

            TextureDesc shadowDepthTexDesc{g_Config->shadowDepthWidth, g_Config->shadowDepthHeight, 0, TextureType::TEXTURE_2D, TextureFormat::DEPTH_COMPONENT32F,
                                        NEAREST_CLAMP_TO_EDGE_WHITE_BORDER_SAMPLER, 0, nullptr, DataFormat::DataFormat_DEPTH_COMPONENT,  DataType::FLOAT};
            cache.shadowPassDepthTexIds = (generateTexture(shadowDepthTexDesc)).texId;
        }
    }
}

void Scene::UploadDataToGpu()
{
    // flag of path tracing
    if(g_Config->lightMode == LightMode::PathTracing)
    {
        TextureDesc verticeTexDesc{vertices.size() * sizeof(vec3), 0, 0, TextureType::Buffer, TextureFormat::RGB32F,
                                NEAREST_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 0, vertices.data()};
        verticeTex = generateTexture(verticeTexDesc);

        TextureDesc indiceTexDesc{indices.size() * sizeof(Indice), 0, 0, TextureType::Buffer, TextureFormat::RGB32I,
                                NEAREST_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 0, indices.data()};
        indicesTex = generateTexture(indiceTexDesc);

        TextureDesc normalTexDesc{normals.size() * sizeof(vec3), 0, 0, TextureType::Buffer, TextureFormat::RGB32F,
                                NEAREST_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 0, normals.data()};
        normalTex = generateTexture(normalTexDesc);

        TextureDesc uvTexDesc{uvs.size() * sizeof(vec2), 0, 0, TextureType::Buffer, TextureFormat::RG32F,
                                NEAREST_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 0, uvs.data()};
        uvsTex = generateTexture(uvTexDesc);

        TextureDesc bvhTexDesc{bvhTranslator.nodes.size() * sizeof(BvhTranslator::Node), 0, 0, TextureType::Buffer, TextureFormat::RGB32F,
                                NEAREST_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 0, bvhTranslator.nodes.data()};
        bvhTex = generateTexture(bvhTexDesc);

        TextureDesc textureArrayDesc{g_Config->texWidth, g_Config->texHeight, textures.size(), TextureType::TEXTURE_2D_ARRAY, TextureFormat::RGBA8,
                            LINEAR_REPEAT_SAMPLER, 0, textureMapsArray.data(), DataFormat::DataFormat_RGBA, DataType::UNSIGNED_BYTE};
        textureArrayTex = generateTexture(textureArrayDesc);
    }
    else
    {
        TextureDesc textureArrayDesc{g_Config->texWidth, g_Config->texHeight, textures.size(), TextureType::TEXTURE_2D_ARRAY, TextureFormat::RGBA8,
                            LINEAR_REPEAT_SAMPLER, 0, textureMapsArray.data(), DataFormat::DataFormat_RGBA, DataType::UNSIGNED_BYTE};
        textureArrayTex = generateTexture(textureArrayDesc);

        for(auto& meshBatch : meshBatches)
        {
            meshBatch->Build();
        }
    }
}
