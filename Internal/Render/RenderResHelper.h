#pragma once
#include <glad/glad.h>
#include <iostream>
#include "Core/Texture.h"
#include "Util/Types.h"

enum class TextureType
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

GLuint generateFBO();

void deleteFBO(GLuint fboId);

GPUTexture generateTexture(const TextureDesc& desc);

void deleteTexture(GPUTexture tex);

class RenderResHelper
{
public:
    static GPUTexture generateGPUTexture(TextureInfo& textureInfo);
    static void destroyGPUTexture(GPUTexture& textureInfo);
    static GPUTexture generateCubeEnvMap(const std::vector<Texture>& envMap);
    static GPUTexture generateIrradianceMap(const GPUTexture& gpuTex);
};

