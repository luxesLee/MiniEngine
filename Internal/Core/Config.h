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

enum ToneMappingType
{
    None,
    Linear,
    ACES,
    TonyMcMapface
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
    uint32_t wholeWidth = 1630;
    const uint32_t imguiWidth = 350;
    uint32_t screenWidth = 1280;
    uint32_t screenHeight = 960;
    std::string title = "MiniEngine";

    // sceneConfigPath
    std::string configPath = "../../Resource/Cornell_box.json";

    // camera
    float cameraMoveSpeed = 7.0f;
    float cameraRotSensitivity = 0.1f;
    float cameraZNear = 800.0f;
    float cameraZFar = 1.0f;

    // render
    LightMode lightMode = LightMode::PathTracing;
    int maxRayTracingDepth = 2;
    uint32_t accumulateFrames = 0;
    DenoiseType curDenoise = DenoiseType::NONE;
    ToneMappingType curToneMapping = ToneMappingType::Linear;
    uint32_t texWidth = 512, texHeight = 512;

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