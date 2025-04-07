#pragma once
#include <cstdint>
#include <string>
#include <glm.hpp>

enum LightMode
{
    Forward,
    Deferred,
    TiledDeferred,
    ClusterDeferred,
    PathTracing,
};

enum DenoiseType
{
    NONE,
    SVGF,
    ODIN,
    OPTIX,
};

class Config
{
public:
    static Config* getInstance()
    {
        static Config instance;
        return &instance;
    }

    // screen
    uint32_t screenWidth = 1280;
    uint32_t screenHeight = 960;
    std::string title = "MiniEngine";

    // model
    std::string initModelPath = "../../Resource/Model/Sponza/glTF/Sponza.gltf";

    // camera
    glm::vec3 initCameraPos = glm::vec3(7.72f, 2.5f, -1.0f);
    glm::vec3 initCameraLookAt = glm::vec3(6.72f, 2.5f, -1.0f);
    float initCameraZoom = 45.0f;

    float cameraMoveSpeed = 7.0f;
    float cameraRotSensitivity = 0.1f;
    float cameraZNear = 800.0f;
    float cameraZFar = 1.0f;

    // render
    LightMode lightMode = LightMode::PathTracing;
    int maxRayTracingDepth = 1;
    uint32_t accumulateFrames = 0;
    DenoiseType curDenoise = DenoiseType::NONE;
    uint32_t texWidth = 2048, texHeight = 2048;

    // action
    bool bRenderdocCapture = false;
    bool bShadeBaseColor = false;

    bool bMouseMove = false;

private:
    Config() {}
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
};

#define g_Config Config::getInstance()