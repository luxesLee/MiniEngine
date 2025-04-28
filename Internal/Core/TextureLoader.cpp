#include "TextureLoader.h"

Uint TextureLoader::LoadTexture(std::string_view path)
{
    std::string textureName;
    if(auto it = loadedTextures.find(textureName); it == loadedTextures.end())
    {
        // 读取图像


        // 生成GPU端图像
        GLuint texId;


        loadedTextures.insert({textureName, texId});
        return texId;
    }
    else
    {
        return it->second;
    }
}

Uint TextureLoader::LoadCubeMap(const std::array<std::string, 6> &paths)
{


    return 0;
}

void TextureLoader::Clear()
{

}
