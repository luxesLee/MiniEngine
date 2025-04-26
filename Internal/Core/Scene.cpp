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
        prepareTransform();
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
    DeleteFBO();
    DeleteGPUData();
    
    vertices.resize(0);
    indices.resize(0);
    normals.resize(0);
    uvs.resize(0);
    materials.resize(0);
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
    transforms.resize(0);
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
    materials.push_back(material);

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

    MeshInstance meshInstance(meshes.size() - 1, 
                                materials.size() - 1, 
                                glm::mat4(1.0f));
    meshInstances.push_back(meshInstance);
}

void Scene::CombineMesh()
{
    std::sort(meshInstances.begin(), meshInstances.end(), [&](MeshInstance& instance1, MeshInstance& instance2)
    {
        return instance1.meshID != instance2.meshID ? instance1.meshID < instance2.meshID : instance1.materialID < instance2.materialID;
    });

    auto CopyData = [&](MeshBatch* meshBatch, auto& meshes, auto& meshInstances, auto& transforms, int index)
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
        transforms.push_back(meshInstances[index].transform);
    };

    MeshBatch* meshBatch = new MeshBatch;
    Int pre = 0;
    CopyData(meshBatch, meshes, meshInstances, transforms, pre);
    for(Int i = 1; i < meshInstances.size(); i++)
    {
        if(meshInstances[i].meshID != meshInstances[pre].meshID)
        {
            meshBatches.push_back(meshBatch);
            meshBatch = new MeshBatch;
        }
       CopyData(meshBatch, meshes, meshInstances, transforms, i);
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

void Scene::prepareTransform()
{
    transforms.resize(meshInstances.size());
#pragma omp parallel for
    for(int i = 0; i < meshInstances.size(); ++i)
    {
        transforms[i] = meshInstances[i].transform;
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
        irradianceEnvTex = RenderResHelper::generateIrradianceMap(envTex);
    }
    else
    {

    }
}

// 暂时这样写，后续用rendergraph
void Scene::InitFBO()
{
    glGenFramebuffers(1, &outputFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);

    glGenTextures(1, &outputTex[0]);
    glBindTexture(GL_TEXTURE_2D, outputTex[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, g_Config->wholeWidth, g_Config->screenHeight, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &outputTex[1]);
    glBindTexture(GL_TEXTURE_2D, outputTex[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, g_Config->wholeWidth, g_Config->screenHeight, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint outputDepthTexId;
    glGenTextures(1, &outputDepthTexId);
    glBindTexture(GL_TEXTURE_2D, outputDepthTexId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, g_Config->screenWidth, g_Config->screenHeight, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, outputDepthTexId, 0);

// ----------------------------------------------------------------------------

    if(g_Config->lightMode == LightMode::PathTracing)
    {
        glGenFramebuffers(1, &pathTracingFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, pathTracingFBO);

        glGenTextures(1, &pathTracingTexId);
        glBindTexture(GL_TEXTURE_2D, pathTracingTexId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, g_Config->screenWidth, g_Config->screenHeight, 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pathTracingTexId, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenTextures(1, &accumTexId);
        glBindTexture(GL_TEXTURE_2D, accumTexId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, g_Config->screenWidth, g_Config->screenHeight, 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, accumTexId, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        GLenum DrawBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, DrawBuffers);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    else if(g_Config->lightMode == LightMode::Deferred)
    {
        glGenFramebuffers(1, &deferredBasePassFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, deferredBasePassFBO);

        glGenTextures(1, &GBufferTexId[0]);
        glBindTexture(GL_TEXTURE_2D, GBufferTexId[0]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, g_Config->screenWidth, g_Config->screenHeight, 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GBufferTexId[0], 0);

        glGenTextures(1, &GBufferTexId[1]);
        glBindTexture(GL_TEXTURE_2D, GBufferTexId[1]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, g_Config->screenWidth, g_Config->screenHeight, 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, GBufferTexId[1], 0);

        glGenTextures(1, &GBufferTexId[2]);
        glBindTexture(GL_TEXTURE_2D, GBufferTexId[2]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, g_Config->screenWidth, g_Config->screenHeight, 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, GBufferTexId[2], 0);

        glGenTextures(1, &GBufferTexId[3]);
        glBindTexture(GL_TEXTURE_2D, GBufferTexId[3]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, g_Config->screenWidth, g_Config->screenHeight, 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, GBufferTexId[3], 0);

        glGenTextures(1, &GBufferTexId[4]);
        glBindTexture(GL_TEXTURE_2D, GBufferTexId[4]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, g_Config->screenWidth, g_Config->screenHeight, 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, GBufferTexId[4], 0);

        glBindTexture(GL_TEXTURE_2D, 0);

        GLenum DrawBuffers[4] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
        glDrawBuffers(4, DrawBuffers);

        glGenTextures(1, &basePassDepthStencilTexId);
        glBindTexture(GL_TEXTURE_2D, basePassDepthStencilTexId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, g_Config->screenWidth, g_Config->screenHeight, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, basePassDepthStencilTexId, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

// ----------------------------------------------------------------------------

    if(g_Config->curToneMapping != ToneMappingType::NONEToneMapping)
    {
        glGenFramebuffers(1, &toneMappingFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, toneMappingFBO);

        glGenTextures(1, &toneMappingTexId);
        glBindTexture(GL_TEXTURE_2D, toneMappingTexId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, g_Config->screenWidth, g_Config->screenHeight, 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, toneMappingTexId, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

}

void Scene::DeleteFBO()
{
    glDeleteFramebuffers(1, &outputFBO);
    glDeleteTextures(1, &outputTex[0]);
    glDeleteTextures(1, &outputTex[1]);

    glDeleteFramebuffers(1, &pathTracingFBO);
    glDeleteTextures(1, &pathTracingTexId);
    glDeleteTextures(1, &accumTexId);

    glDeleteFramebuffers(1, &toneMappingFBO);
    glDeleteTextures(1, &toneMappingTexId);
}

void Scene::InitShadowMapFBO()
{
    if(!g_Config->bShadeShadow)
    {
        return;
    }

    glGenFramebuffers(1, &shadowPassFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowPassFBO);

    std::sort(lights.begin(), lights.end(), [&](Light& light1, Light& light2)
    {
        return light1.type < light2.type;
    });

    constexpr Float bordercolor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    shadowMapCaches.resize(lights.size());
    for(Int i = 0; i < lights.size(); i++)
    {
        ShadowMapCache& cache = shadowMapCaches[i];
        cache.light = &lights[i];

        glGenTextures(1, &cache.shadowPassDepthTexIds);
        if(lights[i].type == LightType::DIRECTIONAL_LIGHT && g_Config->bCascadeShadow)
        {
            cache.bMultiMats = true;
            glBindTexture(GL_TEXTURE_2D_ARRAY, cache.shadowPassDepthTexIds);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, g_Config->shadowDepthWidth, g_Config->shadowDepthHeight, 
                g_Config->cascadeLevel + 1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, bordercolor);
        }
        else if(lights[i].type == LightType::POINT_LIGHT)
        {
            cache.bMultiMats = true;
            glBindTexture(GL_TEXTURE_CUBE_MAP, cache.shadowPassDepthTexIds);
            for(Int j = 0; j < 6; j++)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 0, GL_DEPTH_COMPONENT32F, g_Config->shadowDepthWidth, 
                    g_Config->shadowDepthHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            }

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        }
        else
        {
            cache.bMultiMats = false;
            glBindTexture(GL_TEXTURE_2D, cache.shadowPassDepthTexIds);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, g_Config->shadowDepthWidth, g_Config->shadowDepthHeight, 
                0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bordercolor);
        }
    }
}

void Scene::UploadDataToGpu()
{
    // flag of path tracing
    if(g_Config->lightMode == LightMode::PathTracing)
    {
        TextureInfo vertTexInfo(TextureType::Buffer, 
                                vertices.size() * sizeof(vec3), 
                                vertices.data(), 
                                GL_RGB32F);
        verticeTex = RenderResHelper::generateGPUTexture(vertTexInfo);

        TextureInfo indiceTexInfo(TextureType::Buffer,
                                    indices.size() * sizeof(Indice),
                                    indices.data(),
                                    GL_RGB32I);
        indicesTex = RenderResHelper::generateGPUTexture(indiceTexInfo);

        TextureInfo normalTexInfo(TextureType::Buffer, 
                                    normals.size() * sizeof(vec3), 
                                    normals.data(), 
                                    GL_RGB32F);
        normalTex = RenderResHelper::generateGPUTexture(normalTexInfo);

        TextureInfo uvTexInfo(TextureType::Buffer, 
                                uvs.size() * sizeof(vec2), 
                                uvs.data(), 
                                GL_RG32F);
        uvsTex = RenderResHelper::generateGPUTexture(uvTexInfo);

        TextureInfo matTexInfo(TextureType::Image2D, 
                                    GL_RGBA32F, 
                                    materials.size() * sizeof(Material) / sizeof(vec4), 
                                    1, 
                                    GL_RGBA, 
                                    GL_FLOAT, 
                                    materials.data(),
                                    GL_NEAREST,
                                    GL_NEAREST);
        matTex = RenderResHelper::generateGPUTexture(matTexInfo);

        TextureInfo lightTexInfo(TextureType::Buffer,
                                    lights.size() * sizeof(Light),
                                    lights.data(),
                                    GL_RGBA32F);
        lightTex = RenderResHelper::generateGPUTexture(lightTexInfo);

        TextureInfo transformTexInfo(TextureType::Image2D, 
                                    GL_RGBA32F, 
                                    transforms.size() * sizeof(mat4) / sizeof(vec4), 
                                    1, 
                                    GL_RGBA, 
                                    GL_FLOAT, 
                                    transforms.data(),
                                    GL_NEAREST,
                                    GL_NEAREST);
        instanceTransformTex = RenderResHelper::generateGPUTexture(transformTexInfo);

        TextureInfo bvhTexInfo(TextureType::Buffer, 
                                bvhTranslator.nodes.size() * sizeof(BvhTranslator::Node), 
                                bvhTranslator.nodes.data(), 
                                GL_RGB32F);
        bvhTex = RenderResHelper::generateGPUTexture(bvhTexInfo);

        TextureInfo textureArrayInfo(TextureType::Image3DTextureArray2D,
                                        GL_RGBA8,
                                        g_Config->texWidth,
                                        g_Config->texHeight,
                                        textures.size(),
                                        GL_RGBA,
                                        GL_UNSIGNED_BYTE,
                                        textureMapsArray.data(),
                                        GL_LINEAR,
                                        GL_LINEAR);
        textureArrayTex = RenderResHelper::generateGPUTexture(textureArrayInfo);
    }
    else
    {
        TextureInfo matTexInfo(TextureType::Image2D, 
                                GL_RGBA32F, 
                                materials.size() * sizeof(Material) / sizeof(vec4), 
                                1, 
                                GL_RGBA, 
                                GL_FLOAT, 
                                materials.data(),
                                GL_NEAREST,
                                GL_NEAREST);
        matTex = RenderResHelper::generateGPUTexture(matTexInfo);

        TextureInfo transformTexInfo(TextureType::Image2D, 
                                    GL_RGBA32F, 
                                    transforms.size() * sizeof(mat4) / sizeof(vec4), 
                                    1, 
                                    GL_RGBA, 
                                    GL_FLOAT, 
                                    transforms.data(),
                                    GL_NEAREST,
                                    GL_NEAREST);
        instanceTransformTex = RenderResHelper::generateGPUTexture(transformTexInfo);

        TextureInfo lightTexInfo(TextureType::Buffer,
                                    lights.size() * sizeof(Light),
                                    lights.data(),
                                    GL_RGBA32F);
        lightTex = RenderResHelper::generateGPUTexture(lightTexInfo);

        TextureInfo textureArrayInfo(TextureType::Image3DTextureArray2D,
                                        GL_RGBA8,
                                        g_Config->texWidth,
                                        g_Config->texHeight,
                                        textures.size(),
                                        GL_RGBA,
                                        GL_UNSIGNED_BYTE,
                                        textureMapsArray.data(),
                                        GL_LINEAR,
                                        GL_LINEAR);
        textureArrayTex = RenderResHelper::generateGPUTexture(textureArrayInfo);

        TextureInfo LTC1TexInfo(TextureType::Image2D,
                                    GL_RGBA,
                                    64,
                                    64,
                                    GL_RGBA,
                                    GL_FLOAT,
                                    (void*)LTC1,
                                    GL_NEAREST,
                                    GL_LINEAR);
        LTC1Tex = RenderResHelper::generateGPUTexture(LTC1TexInfo);

        TextureInfo LTC2TexInfo(TextureType::Image2D,
                                    GL_RGBA,
                                    64,
                                    64,
                                    GL_RGBA,
                                    GL_FLOAT,
                                    (void*)LTC2,
                                    GL_NEAREST,
                                    GL_LINEAR);
        LTC2Tex = RenderResHelper::generateGPUTexture(LTC2TexInfo);

        for(auto& meshBatch : meshBatches)
        {
            meshBatch->Build();
        }
    }
}

void Scene::DeleteGPUData()
{
    if(g_Config->lightMode == LightMode::PathTracing)
    {
        RenderResHelper::destroyGPUTexture(verticeTex);
        RenderResHelper::destroyGPUTexture(indicesTex);
        RenderResHelper::destroyGPUTexture(normalTex);
        RenderResHelper::destroyGPUTexture(uvsTex);
        RenderResHelper::destroyGPUTexture(lightTex);
        RenderResHelper::destroyGPUTexture(matTex);
        RenderResHelper::destroyGPUTexture(instanceTransformTex);
        RenderResHelper::destroyGPUTexture(bvhTex);
        RenderResHelper::destroyGPUTexture(textureArrayTex);
    }

    RenderResHelper::destroyGPUTexture(envTex);
    RenderResHelper::destroyGPUTexture(irradianceEnvTex);
}
