#include "ShaderManager.h"
#include "Core/Shader.h"

void ShaderManager::InitShader()
{
    shaderMap["TestShader"] = new Shader("Shaders/TestShader.vert", "Shaders/TestShader.frag");
    shaderMap["PathTracing"] = new Shader("Shaders/PostProcess.vert", "Shaders/PathTracing.frag");
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
