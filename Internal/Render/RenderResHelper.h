#pragma once
#include <glad/glad.h>
#include <cstdint>
#include <iostream>

enum TextureType
{
    Buffer,
    Image2D,
    Image3D,
    Image3DTextureArray2D,
};

struct TextureInfo
{
    TextureInfo(TextureType _texType, uint32_t _dataSize, void* _data, GLenum _internalFormat)
        : texType(_texType), dataSize(_dataSize), data(_data), internalFormat(_internalFormat)
    {
    }

    TextureInfo(TextureType _texType, GLenum _internalFormat, uint32_t _width, uint32_t _height, GLenum _format, GLenum _type, void* _data, GLuint _minFilter, GLuint _magFilter)
        : texType(_texType), internalFormat(_internalFormat), width(_width), height(_height), format(_format), type(_type), data(_data), minFilter(_minFilter), magFilter(_magFilter)
    {
    }

    TextureInfo(TextureType _texType, GLenum _internalFormat, uint32_t _width, uint32_t _height, uint32_t _dataSize, GLenum _format, GLenum _type, void* _data, GLuint _minFilter, GLuint _magFilter)
        : texType(_texType), internalFormat(_internalFormat), width(_width), height(_height), dataSize(_dataSize), format(_format), type(_type), data(_data), minFilter(_minFilter), magFilter(_magFilter)
    {
    }

    TextureInfo()
    {
    }

    TextureType texType;
    uint32_t dataSize;
    uint32_t width;
    uint32_t height;
    void* data;
    GLenum internalFormat;
    GLenum format;
    GLenum type;
    GLuint minFilter, magFilter;
};

struct GPUTexture
{
    GLuint texId;
    GLuint texBufferId;
};

void checkGLError();

class RenderResHelper
{
public:
    static void generateFBO();
    static void deleteFBO();
    static GPUTexture generateGPUTexture(TextureInfo& textureInfo);
    static void destroyGPUTexture(GPUTexture& textureInfo);

};

