#pragma once

#include <string>

enum ImageFormat
{
    DDS,
    STB,
    OTHERS,
};

class Image
{
public:
    explicit Image(const std::string& path);

    bool isInit() const {return bInit;}
    uint32_t Width() const {return width;}
    uint32_t Height() const {return height;}
    char* Data() {return data;}

private:
    void loadDDS(const std::string& path);
    void loadSTB(const std::string& path);

    ImageFormat GetImageFormat(const std::string& path);

private:
    bool bInit = false;
    uint32_t width = 0;
    uint32_t height = 0;
    char* data = nullptr;
};