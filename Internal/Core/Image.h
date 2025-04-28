#pragma once
#include <string>
#include <vector>
#include "Util/Types.h"

enum ImageFormat
{
    DDS,
    STB,
    OTHERS,
};

// Data指针由外部释放
class Image
{
public:
    explicit Image(const std::string& path);

    Bool isInit() const {return bInit;}
    Uint32 Width() const {return width;}
    Uint32 Height() const {return height;}
    unsigned char* Data() {return data.data();}

private:
    Bool loadDDS(const std::string& path);
    Bool loadSTB(const std::string& path);

    ImageFormat GetImageFormat(const std::string& path);

private:
    Bool bInit = false;
    Bool bHdr = false;
    Uint32 width = 0;
    Uint32 height = 0;
    std::vector<Uint8> data;
};