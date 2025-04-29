#pragma once
#include <string>
#include <vector>
#include "glad/glad.h"
#include "Util/Types.h"

enum FilterMode
{
    NEAREST = GL_NEAREST,
    LINEAR = GL_LINEAR,
    NEAREST_MIPMAP_NEAREST = GL_NEAREST_MIPMAP_NEAREST,
    LINEAR_MIPMAP_NEAREST = GL_LINEAR_MIPMAP_NEAREST,
    NEAREST_MIPMAP_LINEAR = GL_NEAREST_MIPMAP_LINEAR,
    LINEAR_MIPMAP_LINEAR = GL_LINEAR_MIPMAP_LINEAR,
};
enum WrapMode
{
    REPEAT = GL_REPEAT,
    MIRRORED_REPEAT = GL_MIRRORED_REPEAT,
    CLAMP_TO_EDGE = GL_CLAMP_TO_EDGE,
    CLAMP_TO_BORDER = GL_CLAMP_TO_BORDER,
};
enum class Bordercolor
{
    BLACK = 0,
    WHITE,
};
struct SamplerDesc
{
    FilterMode filterMin{FilterMode::NEAREST};
    FilterMode filterMag{FilterMode::NEAREST};

    WrapMode warpS{WrapMode::CLAMP_TO_EDGE};
    WrapMode warpT{WrapMode::CLAMP_TO_EDGE};
    WrapMode warpR{WrapMode::CLAMP_TO_EDGE};

    Bordercolor borderColor{Bordercolor::BLACK};
};
const SamplerDesc NEAREST_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER = {};
const SamplerDesc NEAREST_REPEAT_SAMPLER = {FilterMode::NEAREST, FilterMode::NEAREST, WrapMode::REPEAT, WrapMode::REPEAT, WrapMode::REPEAT};
const SamplerDesc LINEAR_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER = {FilterMode::LINEAR, FilterMode::LINEAR};
const SamplerDesc LINEAR_REPEAT_SAMPLER = {FilterMode::LINEAR, FilterMode::LINEAR, WrapMode::REPEAT, WrapMode::REPEAT, WrapMode::REPEAT};
const SamplerDesc LINEAR_MIPMAP_LINEAR_LINEAR_CLAMP_TO_BORDER_BLACK_BORDER_SAMPLER = {FilterMode::LINEAR_MIPMAP_LINEAR, FilterMode::LINEAR, WrapMode::CLAMP_TO_BORDER, WrapMode::CLAMP_TO_BORDER, WrapMode::CLAMP_TO_BORDER};

enum TextureType1 : Uint
{
    TEXTURE_2D          = GL_TEXTURE_2D,
    TEXTURE_2D_ARRAY    = GL_TEXTURE_2D_ARRAY,
    TEXTURE_3D          = GL_TEXTURE_3D,
    TEXTURE_CUBE_MAP    = GL_TEXTURE_CUBE_MAP,
    Buffer              = GL_TEXTURE_BUFFER,
};
enum TextureFormat
{
    RED = GL_RED,
    RG32F = GL_RG32F,
    RGB8 = GL_RGB8,
    RGB32I = GL_RGB32I,
    RGB32F = GL_RGB32F,
    RGBA8 = GL_RGBA8,
    RGBA32F = GL_RGBA32F,
    DEPTH24_STENCIL8 = GL_DEPTH24_STENCIL8,
};
enum DataType
{
    FLOAT = GL_FLOAT,
    UNSIGNED_BYTE = GL_UNSIGNED_BYTE,
    UNSIGNED_INT_24_8 = GL_UNSIGNED_INT_24_8,
};
enum DataFormat
{
    DataFormat_RED = GL_RED,
    DataFormat_RGB = GL_RGB,
    DataFormat_RGBA = GL_RGBA,
    DataFormat_DEPTH_STENCIL = GL_DEPTH_STENCIL,
};
struct TextureDesc
{
    Uint32 width{0};
    Uint32 height{0};
    Uint32 depth{0};
    TextureType1 type{TextureType1::TEXTURE_2D};
    TextureFormat format{TextureFormat::RGBA8};
    SamplerDesc samplerDesc;
    Uint mipLevel{0};

    void* data{nullptr};
    DataFormat dataFormat{DataFormat::DataFormat_RGBA};
    DataType dataType{DataType::UNSIGNED_BYTE};
};

struct Texture
{
    Texture() : width(0), height(0), components(0) 
    {
    }
    Texture(int _width, int _height, int _components, std::string _texName, Uint8* _data)
        : width(_width), height(_height), components(_components), texName(_texName)
    {
        Int32 size = _width * _height * _components;
        data.resize(size);
        memcpy_s(data.data(), size, _data, size);
    }

    Int32 width;
    Int32 height;
    Int32 components;
    std::string texName;
    std::vector<Uint8> data;
};
