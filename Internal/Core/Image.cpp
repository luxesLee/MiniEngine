#include <filesystem>
#include "Image.h"
// #define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Image::Image(const std::string &path)
{
    ImageFormat format = GetImageFormat(path);
    if(format == ImageFormat::DDS)
    {
        bInit = loadDDS(path);
    }
    else if(format == ImageFormat::STB)
    {
        bInit = loadSTB(path);
    }
}

Bool Image::loadDDS(const std::string &path)
{
    return false;
}

Bool Image::loadSTB(const std::string &path)
{
    bHdr = stbi_is_hdr(path.data());
    Int32 _Width, _Height;
    if(bHdr)
    {
        Float* _data = stbi_loadf(path.data(), &_Width, &_Height, nullptr, 4);
        if(!_data)
        {
            return false;
        }
        width = _Width;
        height = _Height;
        Uint32 dataSize = 4 * sizeof(Float) * width * height;
        data.resize(dataSize);
        memcpy_s(data.data(), dataSize, _data, dataSize);
        stbi_image_free(_data);
    }
    else
    {
        unsigned char* _data = stbi_load(path.data(), &_Width, &_Height, nullptr, 4);
        if(!_data)
        {
            return false;
        }
        width = _Width;
        height = _Height;
        Uint32 dataSize = 4 * width * height;
        data.resize(dataSize);
        memcpy_s(data.data(), dataSize, _data, dataSize);
        stbi_image_free(_data);
    }
    return true;
}

ImageFormat Image::GetImageFormat(const std::string &path)
{
    std::filesystem::path p(path);
    std::string extension = p.extension().string();
    
    if(extension == ".dds")
    {
        return ImageFormat::DDS;
    }
    else if(extension == ".bmp" || extension == ".jpg" || extension == ".jpeg" || extension == ".png" 
        || extension == ".tiff" || extension == ".tif" || extension == ".gif" || extension == ".tga" || extension == ".hdr")
    {
        return ImageFormat::STB;
    }
    return ImageFormat::OTHERS;
}
