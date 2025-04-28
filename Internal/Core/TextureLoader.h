#pragma once
#include <string>
#include <unordered_map>
#include "glad/glad.h"
#include "Util/Singleton.h"
#include "Util/Types.h"


class TextureLoader : public Singleton<TextureLoader>
{
    friend class Singleton<TextureLoader>;
    using TextureName = std::string;

public:
    Uint LoadTexture(std::string_view path);
    Uint LoadCubeMap(const std::array<std::string, 6>& paths);
    void Clear();

private:
    TextureLoader() {}
    ~TextureLoader() {}

private:
    std::unordered_map<TextureName, Uint> loadedTextures;
    
};
#define g_TextureLoader TextureLoader::GetInstnace()
