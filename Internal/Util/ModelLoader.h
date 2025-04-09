#pragma once
#include <entt.hpp>
#include <vector>
#include <tiny_gltf.h>

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
    void loadLight(Scene* scene, const std::string& filePath);
    entt::entity loadModel(Scene *scene, const std::string& filePath);

private:

    entt::entity loadGLTFModel(Scene *scene, const std::string& filePath);
    void loadMatFromGLTFModel(Scene* scene, tinygltf::Model& gltfModel);
    void loadMeshFromGLTFModel(Scene* scene, tinygltf::Model& gltfModel, std::map<int, std::vector<std::pair<int, int>>>& meshMap);
    void loadTexturesFromGLTFModel(Scene* scene, tinygltf::Model& gltfModel);
    void loadInstanceFromGLTFModel(Scene* scene, tinygltf::Model& gltfModel, std::map<int, std::vector<std::pair<int, int>>>& meshMap);

    entt::entity loadOBJModel();

private:
    entt::registry& reg;
};
