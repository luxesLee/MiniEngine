#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "vec3.hpp"
#include "mat4x4.hpp"
#include "Core/Material.h"
#include "Util/Types.h"

struct ModelConfig
{
    std::string modelPath;
    glm::mat4 transform;
    std::string materialName;
};

struct EnvMapConfig
{
    std::vector<std::string> envMapPaths;
    Bool bCubeMap;
};

enum LightType
{
    DIRECTIONAL_LIGHT,
    POINT_LIGHT,
    SPOT_LIGHT,
    QUAD_LIGHT,
};

struct LightConfig
{
    glm::vec4 position;
    glm::vec4 direction;
    glm::vec4 color;

    int active;
    float range;
    LightType type;
    float outerCosine;

    float innerCosine;
    glm::vec3 u;
    
	glm::vec3 v;
    int     padd;
};

struct CameraConfig
{
    glm::vec3 position;
    glm::vec3 lookAt;
    float zoom;
    float zFar;
    float zNear;
};

struct MaterialConfig
{
    Material mat;
    std::string matPath;
    bool bNotLoad;
};

typedef std::unordered_map<std::string, MaterialConfig> MaterialConfigMap;
struct SceneConfig
{
    std::vector<ModelConfig> modelConfigs;
    std::vector<LightConfig> lightConfigs;
    CameraConfig cameraConfig;
    EnvMapConfig envMapConfig;
    MaterialConfigMap matConfigMap;
};

void ParseConfig(const std::string& configFile, SceneConfig& config);
bool DumpConfig(const std::string& configFile, SceneConfig& config);