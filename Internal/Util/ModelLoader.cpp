#include "ModelLoader.h"
#include "Core/Scene.h"
#include "Core/Mesh.h"
#include "Core/Texture.h"
#include "Core/Image.h"
#define TINYOBJLOADER_IMPLEMENTATION 
#include <tiny_obj_loader.h>
#include <string>
#include <iostream>
#include <map>
#include <functional>
#include <utility>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

void ModelLoader::loadEnvMap(Scene *scene, const std::string &filePath)
{
    // Image image(filePath);
    // if(image.isInit())
    {
        // Texture* envMapTex = new Texture(image.Width(), image.Height());

        if(scene->envMap)
        {
            delete scene->envMap;
            scene->envMap = nullptr;
        }
        // scene->envMap = envMapTex;
    }
}

void ModelLoader::loadLight(Scene *scene, const std::string &filePath)
{
    if(filePath.empty())
    {
        // load a default light
        Light light;
        light.position = vec4(0, 0, 0, 0);
        light.direction = vec4(0, -0.9563, -0.2924, 0);
        light.color = vec4(7.15, 633478, 6.0917, 6.5);
        light.active = 1;
        light.range = 100;
        light.type = DIRECTIONAL_LIGHT;
        light.outerCosine = 0.707;
        light.innerCosine = 0.9238;
        light.volumetric = 1;
        light.volumetricStrength = 0.004;
        light.useCascades = 1;
        scene->lights.push_back(light);
    }
}

entt::entity ModelLoader::loadModel(Scene *scene, const std::string& filePath)
{
    auto endsWith = [=](const std::string& str, const std::string& compare)
    {
        int n1 = str.size() - 1, n2 = compare.size() - 1;
        if(n2 > n1)
        {
            return false;
        }
        while(n2 >= 0 && str[n1] == compare[n2])
        {
            --n1;
            --n2;
        }
        return n2 < 0;
    };

    if(endsWith(filePath, ".obj"))
    {
        return loadOBJModel();
    }
    else if(endsWith(filePath, ".gltf"))
    {
        return loadGLTFModel(scene, filePath);
    }
    return entt::null;
}

entt::entity ModelLoader::loadGLTFModel(Scene *scene, const std::string& filePath)
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model gltfModel;
    std::string err, warn;

    bool ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filePath);
    if(!ret || !err.empty())
    {
        if(!err.empty())
        {
            std::cout << "TINYGLTF Error: " << err << std::endl;
        }
        return entt::null;
    }
    if(!warn.empty())
    {
        std::cout << "TINYGLTF Warn: " << warn << std::endl;
    }

    // <MeshId in gltfModel, <MeshId in scene, MatId in scene>>
    std::map<int, std::vector<std::pair<int, int>>> meshMap;
    // cannot revert loadMesh and loadMat
    // for MatId in scene = num of mat in scene + MatId in gltfModel
    loadMeshFromGLTFModel(scene, gltfModel, meshMap);
    loadMatFromGLTFModel(scene, gltfModel);
    loadTexturesFromGLTFModel(scene, gltfModel);
    loadInstanceFromGLTFModel(scene, gltfModel, meshMap);

    // entt::entity entityModel = reg.create();
    return entt::null;
}

void ModelLoader::loadMatFromGLTFModel(Scene *scene, tinygltf::Model& gltfModel)
{
    for(const auto& mat : gltfModel.materials)
    {
        const tinygltf::PbrMetallicRoughness pbr = mat.pbrMetallicRoughness;

        Material material;

        material.baseColor = Vec3((float)pbr.baseColorFactor[0], (float)pbr.baseColorFactor[1], (float)pbr.baseColorFactor[2]);
        if (pbr.baseColorTexture.index > -1)
            material.baseColorTexId = pbr.baseColorTexture.index + scene->textures.size();
        
        material.opacity = (float)pbr.baseColorFactor[3];

        material.alphaCutoff = static_cast<float>(mat.alphaCutoff);

        material.roughness = sqrtf((float)pbr.roughnessFactor);
        material.metallic = (float)pbr.metallicFactor;
        if (pbr.metallicRoughnessTexture.index > -1)
            material.metallicRoughnessTexID = pbr.metallicRoughnessTexture.index + scene->textures.size();

        material.normalmapTexID = mat.normalTexture.index + scene->textures.size();

        material.emission = vec3(mat.emissiveFactor[0], mat.emissiveFactor[1], mat.emissiveFactor[2]);
        if (mat.emissiveTexture.index > -1)
            material.emissionmapTexID = mat.emissiveTexture.index + scene->textures.size();

        if (mat.extensions.find("KHR_materials_transmission") != mat.extensions.end())
        {
            const auto& ext = mat.extensions.find("KHR_materials_transmission")->second;
            if (ext.Has("transmissionFactor"))
                material.specTrans = (float)(ext.Get("transmissionFactor").Get<double>());
        }
        
        scene->materials.push_back(material);
    }

    if(gltfModel.materials.size() == 0)
    {
        static Material defaultMat;
        scene->materials.push_back(defaultMat);
    }
}

void ModelLoader::loadMeshFromGLTFModel(Scene *scene, tinygltf::Model& gltfModel, std::map<int, std::vector<std::pair<int, int>>>& meshMap)
{
    for(int meshId = 0; meshId < gltfModel.meshes.size(); meshId++)
    {
        auto mesh = gltfModel.meshes[meshId];
        for(const auto& prim : mesh.primitives)
        {
            if(prim.mode != TINYGLTF_MODE_TRIANGLES)
            {
                continue;
            }

            Mesh* mesh = new Mesh();

            std::vector<glm::vec3> vertices;
            std::vector<glm::vec3> normals;
            std::vector<glm::vec2> uvs;

            // vertices
            const auto& positionAccessor = gltfModel.accessors[prim.attributes.at("POSITION")];
            const auto& positionBufferView = gltfModel.bufferViews[positionAccessor.bufferView];
            const auto& positionBuffer = gltfModel.buffers[positionBufferView.buffer];
            size_t positionCount = positionAccessor.count;
            vertices.resize(positionCount);
            // suppose the vertice component in model is 3
            memcpy_s(vertices.data(), positionCount * sizeof(vec3), 
                positionBuffer.data.data() + positionBufferView.byteOffset + positionAccessor.byteOffset, positionCount * sizeof(vec3));

            const auto& indexAccessor = gltfModel.accessors[prim.indices];
            const auto& indexBufferView = gltfModel.bufferViews[indexAccessor.bufferView];
            const auto& indexBuffer = gltfModel.buffers[indexBufferView.buffer];
            size_t indexCount = indexAccessor.count;
            
            // normal
            if(prim.attributes.count("NORMAL") > 0)
            {
                const auto& normalAccessor = gltfModel.accessors[prim.attributes.at("NORMAL")];
                const auto& normalBufferView = gltfModel.bufferViews[normalAccessor.bufferView];
                const auto& normalBuffer = gltfModel.buffers[normalBufferView.buffer];
                size_t normalCount = normalAccessor.count;
                normals.resize(normalCount);
                memcpy_s(normals.data(), normalCount * sizeof(vec3),
                    normalBuffer.data.data() + normalBufferView.byteOffset + normalAccessor.byteOffset, normalCount * sizeof(vec3));
            }

            // uv
            if(prim.attributes.count("TEXCOORD_0") > 0)
            {
                const auto& uvAccessor = gltfModel.accessors[prim.attributes.at("TEXCOORD_0")];
                const auto& uvBufferView = gltfModel.bufferViews[uvAccessor.bufferView];
                const auto& uvBuffer = gltfModel.buffers[uvBufferView.buffer];
                size_t uvCount = uvAccessor.count;
                uvs.resize(uvCount);
                memcpy_s(uvs.data(), uvCount * sizeof(vec2),
                    uvBuffer.data.data() + uvBufferView.byteOffset + uvAccessor.byteOffset, uvCount * sizeof(vec2));
            }

            const uint8_t* indexBufferAddress = indexBuffer.data.data();
            int indexStride = tinygltf::GetComponentSizeInBytes(indexAccessor.componentType) * tinygltf::GetNumComponentsInType(indexAccessor.type);

            std::vector<int> indices(indexAccessor.count);
            const uint8_t* baseAddress = indexBufferAddress + indexBufferView.byteOffset + indexAccessor.byteOffset;
            if (indexStride == 1)
            {
                std::vector<uint8_t> quarter;
                quarter.resize(indexAccessor.count);

                memcpy(quarter.data(), baseAddress, (indexAccessor.count * indexStride));

                // Convert quarter precision indices to full precision
                for (size_t i = 0; i < indexAccessor.count; i++)
                {
                    indices[i] = quarter[i];
                }
            }
            else if (indexStride == 2)
            {
                std::vector<uint16_t> half;
                half.resize(indexAccessor.count);

                memcpy(half.data(), baseAddress, (indexAccessor.count * indexStride));

                // Convert half precision indices to full precision
                for (size_t i = 0; i < indexAccessor.count; i++)
                {
                    indices[i] = half[i];
                }
            }
            else
            {
                memcpy(indices.data(), baseAddress, (indexAccessor.count * indexStride));
            }

            // Get triangles from vertex indices
            for (int v = 0; v < indices.size(); v++)
            {
                mesh->vertices.push_back(vertices[indices[v]]);
                mesh->normals.push_back(normals[indices[v]]);
                mesh->uvs.push_back(uvs[indices[v]]);
            }

            scene->meshes.push_back(mesh);
            meshMap[meshId].push_back(std::make_pair<int, int>(scene->meshes.size() - 1, scene->materials.size() + prim.material));
        }
    }
}

void ModelLoader::loadTexturesFromGLTFModel(Scene *scene, tinygltf::Model& gltfModel)
{
    for(const auto& tex : gltfModel.textures)
    {
        auto& img = gltfModel.images[tex.source];
        Texture* texture = new Texture(img.width, img.height, img.component, tex.name, img.image.data());
        scene->textures.push_back(texture);
    }
}

void ModelLoader::loadInstanceFromGLTFModel(Scene *scene, tinygltf::Model& gltfModel, std::map<int, std::vector<std::pair<int, int>>>& meshMap)
{
    std::function<void(int, mat4&)> travelNodes = [&](int nodeId, mat4& parentMat)
    {
        auto gltfNode = gltfModel.nodes[nodeId];
        mat4 localMat = mat4(1.0f), matTmp;
        if(gltfNode.matrix.size() > 0)
        {
            localMat = make_mat4((float*)gltfNode.matrix.data());
        }
        else
        {
            if(gltfNode.translation.size() > 0)
            {
                // glm col
                localMat[3] = vec4(make_vec3((float*)gltfNode.translation.data()), 1.0f);
            }
            if(gltfNode.rotation.size() > 0)
            {
                localMat = mat4_cast(make_quat((float*)gltfNode.rotation.data())) * localMat;
            }
            if(gltfNode.scale.size() > 0)
            {
                scale(localMat, make_vec3((float*)gltfNode.scale.data()));
                localMat[0][0] *= gltfNode.scale[0];
                localMat[1][1] *= gltfNode.scale[1];
                localMat[2][2] *= gltfNode.scale[2];
            }
        }
        matTmp = localMat * parentMat;
        
        if(gltfNode.children.size() == 0 && gltfNode.mesh != -1)
        {
            auto prims = meshMap[gltfNode.mesh];
            for(const auto& prim : prims)
            {
                MeshInstance meshInstance(prim.first, prim.second, matTmp);
                scene->meshInstances.push_back(meshInstance);
            }
        }

        for(int i = 0; i < gltfNode.children.size(); i++)
        {
            travelNodes(gltfNode.children[i], matTmp);
        }
    };
    
    mat4 xform = mat4(1.0f);
    const auto gltfScene = gltfModel.scenes[gltfModel.defaultScene];
    for(int rootId = 0; rootId < gltfScene.nodes.size(); rootId++)
    {
        travelNodes(gltfScene.nodes[rootId], xform);
    }
}

entt::entity ModelLoader::loadOBJModel()
{
    std::string filePath;
    std::string err, warn;   
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str());
    if(!ret || !err.empty())
    {
        if(!err.empty())
        {
            std::cout << "TINYGLTF Error: " << err << std::endl;
        }
        return entt::null;
    }
    if(!warn.empty())
    {
        std::cout << "TINYGLTF Warn: " << warn << std::endl;
    }

    entt::entity entityModel = reg.create();


    return entityModel;
}
