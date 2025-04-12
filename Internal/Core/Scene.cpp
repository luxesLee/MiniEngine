#include "Scene.h"
#include "Camera.h"
#include "Config.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#include "stb_image.h"

void Scene::BuildScene()
{
    if(g_Config->lightMode == LightMode::PathTracing)
    {
        // Bottom Level Acceleration Structure -- for mesh
        createBLAS();
        // Top Level Acceleration Structure -- for meshInstance
        createTLAS();
        bvhTranslator.Process(sceneBvh, meshes, meshInstances);

        prepareMeshData();
        prepareTransform();
        prepareTexture();
    }


    UploadDataToGpu();
}

void Scene::CleanScene()
{
    
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

// 暂时这样写，后续用rendergraph
void Scene::InitFBO()
{
    glGenFramebuffers(1, &outputFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);

    glGenTextures(1, &outputTex[0]);
    glBindTexture(GL_TEXTURE_2D, outputTex[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, g_Config->wholeWidth, g_Config->screenHeight, 0, GL_RGB, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &outputTex[1]);
    glBindTexture(GL_TEXTURE_2D, outputTex[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, g_Config->wholeWidth, g_Config->screenHeight, 0, GL_RGB, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

// ----------------------------------------------------------------------------

    glGenFramebuffers(1, &pathTracingFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, pathTracingFBO);

    glGenTextures(1, &pathTracingTexId);
    glBindTexture(GL_TEXTURE_2D, pathTracingTexId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, g_Config->wholeWidth, g_Config->screenHeight, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pathTracingTexId, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &accumTexId);
    glBindTexture(GL_TEXTURE_2D, accumTexId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, g_Config->wholeWidth, g_Config->screenHeight, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, accumTexId, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLenum DrawBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, DrawBuffers);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

// ----------------------------------------------------------------------------

    glGenFramebuffers(1, &toneMappingFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, toneMappingFBO);

    glGenTextures(1, &toneMappingTexId);
    glBindTexture(GL_TEXTURE_2D, toneMappingTexId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, g_Config->wholeWidth, g_Config->screenHeight, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, toneMappingTexId, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
                                    materials.data());
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
                                    transforms.data());
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
                                        textureMapsArray.data());
        textureArrayTex = RenderResHelper::generateGPUTexture(textureArrayInfo);
    }
    else
    {
        
    }
}