#include "ShaderManager.h"
#include "Core/Shader.h"

void ShaderManager::InitShader()
{
    // PathTracing
    shaderMap["PathTracing"] = new Shader("Shaders/PostProcess.vert", "Shaders/PathTracing.frag");

    // Deferred Rendering
    shaderMap["BasePass"] = new Shader("Shaders/GBufferPass.vert", "Shaders/GBufferPass.frag");
    shaderMap["PreDepthPass"] = new Shader("Shaders/GBufferPass.vert", "Shaders/EmptyFrag.frag");
    shaderMap["DeferredLighting"] = new Shader("Shaders/DeferredLighting.comp");

    // ShadowMap
    shaderMap["DirectionalShadowDepth"] = new Shader("Shaders/Shadow/SimpleShadow.vert", "Shaders/EmptyFrag.frag");
    shaderMap["CascadeDirectionalShadowDepth"] = new Shader("Shaders/Shadow/LayeredShadow.vert", "Shaders/EmptyFrag.frag", "Shaders/Shadow/CascadeShadow.geom");
    shaderMap["PointShadowDepth"] = new Shader("Shaders/Shadow/LayeredShadow.vert", "Shaders/Shadow/PointShadow.frag", "Shaders/Shadow/PointShadow.geom");

    // SSAO
    shaderMap["SSAO"] = new Shader("Shaders/PostProcess.vert", "Shaders/Indirect/SSAO/SSAO.frag");
    shaderMap["SSAOBlur"] = new Shader("Shaders/PostProcess.vert", "Shaders/Indirect/SSAO/SSAOBlur.frag");

    // VXGI
    shaderMap["VoxelScene"] = new Shader("Shaders/Indirect/VXGI/VoxelScene.vert", "Shaders/Indirect/VXGI/VoxelScene.frag", "Shaders/Indirect/VXGI/VoxelScene.geom");
    shaderMap["VXGILightInject"] = new Shader("Shaders/Indirect/VXGI/LightInject.comp");
    shaderMap["VoxelMipmapGenerate"] = new Shader("Shaders/Indirect/VXGI/VoxelMipmapGenerate.comp");
    shaderMap["VXGIIndirectLighting"] = new Shader("Shaders/Indirect/VXGI/VXGIIndirectLighting.comp");

    // Post Process
    shaderMap["ToneMapping"] = new Shader("Shaders/PostProcess.vert", "Shaders/ToneMapping.frag");

    // Util
    shaderMap["IrradianceConv"] = new Shader("Shaders/CubeMap.vert", "Shaders/IrradianceConvolution.frag");

    // Debug
    shaderMap["VisualizeShadowMap"] = new Shader("Shaders/Debug/VisualizeShadowMap.vert", "Shaders/Debug/VisualizeShadowMap.frag");
    shaderMap["VisualizeVoxel"] = new Shader("Shaders/Debug/VisualizeVoxel.vert", "Shaders/Debug/VisualizeVoxel.frag",  "Shaders/Debug/VisualizeVoxel.geom");
    shaderMap["TestShader"] = new Shader("Shaders/Debug/TestShader.vert", "Shaders/Debug/TestShader.frag");

}

Shader* ShaderManager::GetShader(const std::string &shaderKey)
{
    if(shaderMap.find(shaderKey) == shaderMap.end())
    {
        return nullptr;
    }
    return shaderMap[shaderKey];
}

ShaderManager::~ShaderManager()
{
    for(auto it = shaderMap.begin(); it != shaderMap.end(); it++)
    {
        if(it->second)
        {
            delete it->second;
            it->second = nullptr;
        }
    }
}
