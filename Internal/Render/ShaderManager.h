#pragma once
#include "Util/Singleton.h"
#include <unordered_map>
#include <string>

class Shader;



class ShaderManager : public Singleton<ShaderManager>
{
    friend class Singleton<ShaderManager>;
public:
    void InitShader();
    Shader* GetShader(const std::string& shaderKey);

private:
    ShaderManager() = default;
    ~ShaderManager();

private:
    std::unordered_map<std::string, Shader*> shaderMap;
};

#define g_ShaderManager ShaderManager::GetInstnace() 