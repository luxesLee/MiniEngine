#include "Image.h"
#include <filesystem>


Image::Image(const std::string &path)
{
    ImageFormat format = GetImageFormat(path);
    if(format == ImageFormat::DDS)
    {
        loadDDS(path);
    }
    else if(format == ImageFormat::STB)
    {
        loadSTB(path);
    }
}

void Image::loadDDS(const std::string &path)
{

}

void Image::loadSTB(const std::string &path)
{
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
