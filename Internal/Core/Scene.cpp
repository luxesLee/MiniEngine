#include "Scene.h"
#include "Config.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#include "stb_image.h"

void Scene::buildScene()
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
                                    GL_R32F);
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
                                        2048,
                                        2048,
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