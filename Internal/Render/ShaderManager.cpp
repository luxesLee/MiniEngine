#include "ShaderManager.h"
#include "Core/Shader.h"

void ShaderManager::InitShader()
{
    shaderMap["TestShader"] = new Shader("Shaders/TestShader.vert", "Shaders/TestShader.frag");
    shaderMap["PathTracing"] = new Shader("Shaders/PostProcess.vert", "Shaders/PathTracing.frag");

    // Deferred Rendering
    shaderMap["BasePass"] = new Shader("Shaders/GBufferPass.vert", "Shaders/GBufferPass.frag");
    shaderMap["PreDepthPass"] = new Shader("Shaders/GBufferPass.vert", "Shaders/EmptyFrag.frag");
    shaderMap["DeferredLighting"] = new Shader("Shaders/DeferredLighting.comp");

    // ShadowMap
    shaderMap["DirectionalShadowDepth"] = new Shader("Shaders/Shadow/SimpleShadow.vert", "Shaders/EmptyFrag.frag");
    shaderMap["CascadeDirectionalShadowDepth"] = new Shader("Shaders/Shadow/LayeredShadow.vert", "Shaders/EmptyFrag.frag", "Shaders/Shadow/CascadeShadow.geom");
    shaderMap["PointShadowDepth"] = new Shader("Shaders/Shadow/LayeredShadow.vert", "Shaders/Shadow/PointShadow.frag", "Shaders/Shadow/PointShadow.geom");
    shaderMap["VisualizeShadowMap"] = new Shader("Shaders/Shadow/VisualizeShadowMap.vert", "Shaders/Shadow/VisualizeShadowMap.frag");

    // SSAO
    shaderMap["SSAO"] = new Shader("Shaders/PostProcess.vert", "Shaders/Indirect/SSAO/SSAO.frag");
    shaderMap["SSAOBlur"] = new Shader("Shaders/PostProcess.vert", "Shaders/Indirect/SSAO/SSAOBlur.frag");

    // VXGI

    // Post Process
    shaderMap["ToneMapping"] = new Shader("Shaders/PostProcess.vert", "Shaders/ToneMapping.frag");
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
