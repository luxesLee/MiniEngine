#pragma once
#include <entt.hpp>
#include <vector>
#include "tiny_gltf.h"
#include "mat4x4.hpp"
#include "ConfigLoader.h"

class Scene;

class ModelLoader
{
public:
    ModelLoader(entt::registry& _reg)
        : reg(_reg)
    {
    }
    ~ModelLoader()
    {
    }

    void loadEnvMap(Scene* scene, const std::string& filePath);
    void loadEnvMap(Scene* scene, const std::vector<std::string>& filePaths);
    entt::entity loadModel(Scene *scene, const ModelConfig& modelConfig, const MaterialConfigMap& matConfigMap);

private:

    entt::entity loadGLTFModel(Scene *scene, const ModelConfig& modelConfig, const MaterialConfigMap& matConfigMap);
    void loadMatFromGLTFModel(Scene* scene, tinygltf::Model& gltfModel);
    void loadMeshFromGLTFModel(Scene* scene, tinygltf::Model& gltfModel, std::map<int, std::vector<std::pair<int, int>>>& meshMap);
    void loadTexturesFromGLTFModel(Scene* scene, tinygltf::Model& gltfModel);
    void loadInstanceFromGLTFModel(Scene* scene, tinygltf::Model& gltfModel, std::map<int, std::vector<std::pair<int, int>>>& meshMap, const glm::mat4& transform);

    entt::entity loadOBJModel(Scene *scene, const ModelConfig& modelConfig, const MaterialConfigMap& matConfigMap);

private:
    entt::registry& reg;
};
