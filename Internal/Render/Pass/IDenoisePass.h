#pragma once
#include "Core/Config.h"

class IDenoisePass
{
public:
    virtual ~IDenoisePass() = default;
    virtual void AddPass() = 0;
    virtual DenoiseType GetType() const = 0;
};

class DefaultDenoisePass : public IDenoisePass
{
public:
    virtual void AddPass() {}
    virtual DenoiseType GetType() const {return NONE;}
};

class SVGFDenoisePass : public IDenoisePass
{
public:
    virtual void AddPass();
    virtual DenoiseType GetType() const {return SVGF;}
};

class OIDNDenoisePass : public IDenoisePass
{
public:
    virtual void AddPass();
    virtual DenoiseType GetType() const {return ODIN;}
};

class OPTIXDenoisePass : public IDenoisePass
{
public:
    virtual void AddPass();
    virtual DenoiseType GetType() const {return OPTIX;}
};


