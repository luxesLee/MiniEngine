#include "ShaderManager.h"
#include "Core/Shader.h"

void ShaderManager::InitShader()
{
    shaderMap["TestShader"] = new Shader("Shaders/TestShader.vert", "Shaders/TestShader.frag");
    shaderMap["PathTracing"] = new Shader("Shaders/PostProcess.vert", "Shaders/PathTracing.frag");
    shaderMap["ToneMapping"] = new Shader("Shaders/PostProcess.vert", "Shaders/ToneMapping.frag");
    shaderMap["BasePass"] = new Shader("Shaders/GBufferPass.vert", "Shaders/GBufferPass.frag");
    shaderMap["PreDepthPass"] = new Shader("Shaders/GBufferPass.vert", "Shaders/EmptyFrag.frag");
    shaderMap["ShadowDepth"] = new Shader("Shaders/Shadow.vert", "Shaders/EmptyFrag.frag");
    shaderMap["VisualizeShadowMap"] = new Shader("Shaders/VisualizeShadowMap.vert", "Shaders/VisualizeShadowMap.frag");
    shaderMap["DeferredLighting"] = new Shader("Shaders/DeferredLighting.comp");
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
