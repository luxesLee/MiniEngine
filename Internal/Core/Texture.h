#pragma once
#include <string>
#include <vector>

struct Texture
{
    Texture() : width(0), height(0), components(0) 
    {
    }
    Texture(int _width, int _height, int _components, std::string _texName, unsigned char* _data)
        : width(_width), height(_height), components(_components), texName(_texName)
    {
        uint32_t size = _width * _height * _components;
        data.resize(size);
        memcpy_s(data.data(), size, _data, size);
    }

    int width;
    int height;
    int components;
    std::string texName;
    std::vector<unsigned char> data;
};
