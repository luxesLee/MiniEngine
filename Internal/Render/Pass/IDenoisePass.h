#pragma once
#include "Core/Config.h"
#include "OpenImageDenoise/oidn.hpp"
#include <vector>

class Scene;

class IDenoisePass
{
public:
    virtual ~IDenoisePass() = default;
    virtual void AddPass(Scene* scene) = 0;
    virtual DenoiseType GetType() const = 0;
};

class SVGFDenoisePass : public IDenoisePass
{
public:
    SVGFDenoisePass();

    virtual void AddPass(Scene* scene);
    virtual DenoiseType GetType() const {return DenoiseType::SVGF;}
};

class OIDNDenoisePass : public IDenoisePass
{
public:
    OIDNDenoisePass();
    ~OIDNDenoisePass();
    virtual void AddPass(Scene* scene);
    virtual DenoiseType GetType() const {return DenoiseType::ODIN;}

private:
    std::vector<float> inputFrameVec;
    std::vector<float> outputFrameVec;
    oidn::DeviceRef device;
    oidn::FilterRef filter;
};

class OPTIXDenoisePass : public IDenoisePass
{
public:
    virtual void AddPass(Scene* scene);
    virtual DenoiseType GetType() const {return DenoiseType::OPTIX;}
};

IDenoisePass* CreateDenoisePass();
