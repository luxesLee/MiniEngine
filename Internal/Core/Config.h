#pragma once
#include <cstdint>
#include <string>
#include "glm.hpp"
#include "Util/Types.h"

enum class LightMode
{
    Forward,
    Deferred,
    TiledDeferred,
    ClusterDeferred,
    PathTracing,
};

enum class DenoiseType
{
    NONE,
    SVGF,
    ODIN,
    OPTIX,
};

enum class ToneMappingType
{
    NONE,
    Linear,
    ACES,
    TonyMcMapface
};

enum class DebugMode : Uint32
{
    NONE = 1,
    DebugShadow = (1 << 1),
    DebugVXGI = (1 << 2),
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
    Uint32 wholeWidth = 1600;
    const Uint32 imguiWidth = 320;
    Uint32 screenWidth = 1280;
    Uint32 screenHeight = 1024;
    std::string title = "MiniEngine";

    // sceneConfigPath
    std::string configPath = "../../Resource/newsponza.json";
    // std::string configPath = "../../Resource/Cornell_box_Point_Light.json";

    // camera
    Float cameraMoveSpeed = 20.0f;
    Float cameraRotSensitivity = 0.1f;
    Float cameraZNear = 10.0f;
    Float cameraZFar = 0.1f;

    // render
    LightMode lightMode = LightMode::Deferred;
    Int32 maxRayTracingDepth = 2;
    Uint32 accumulateFrames = 0;
    DenoiseType curDenoise = DenoiseType::NONE;
    ToneMappingType curToneMapping = ToneMappingType::Linear;
    Uint32 texWidth = 512, texHeight = 512;

    Bool bPreDepthPass = false;

    // shadow
    Bool bShadeShadow = true;
    Bool bPCF = true;
    Int cascadeLevel = 4;
    Bool bCascadeShadow = false;
    Uint32 shadowDepthWidth = 1024, shadowDepthHeight = 1024;

    // Indirect Lighting
    Bool bSSAO = false;
    Int SSAOKernelSize = 64;
    Float SSAORadius = 0.5f;
    Float SSAOBias = 0.025f;

    Bool bDDGI = false;

    Bool bVXGI = true;
    Int VoxelSize = 256;
    Int VoxelMipmapLvel = 6;
    
    Bool bSSR = true;

    // Debug
    DebugMode debugMode = DebugMode::NONE;

    // action
    Bool bRenderdocCapture = false;
    Bool bShadeBaseColor = false;
    Bool bMouseMove = false;

private:
    Config() {}
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
};

#define g_Config Config::getInstance()