#pragma once
#include "Util/Types.h"
using RenderResourceHandle = Uint32;

struct DefaultData
{
    RenderResourceHandle defaultFBO;
    RenderResourceHandle defaultColorData[2];
    RenderResourceHandle defaultDepthData;
    RenderResourceHandle defaultVAO;
};

struct GBufferData{
    RenderResourceHandle gBufferFBO;
    RenderResourceHandle gBuffer0;
    RenderResourceHandle gBuffer1;
    RenderResourceHandle gBuffer2;
    RenderResourceHandle gBuffer3;
    RenderResourceHandle gBuffer4;  
    RenderResourceHandle gBufferDepthStencil;
};

struct GPUTransformData
{
    RenderResourceHandle transformData;
};

struct GPUMaterialData
{
    RenderResourceHandle matTexData;
};

struct GPULightData
{
    RenderResourceHandle lightData;
};

struct GPUCombinedTextureData
{
    RenderResourceHandle combinedTexData;
};

struct ShadowMapData
{
    RenderResourceHandle shadowPassFBO;

};

struct IrradianceEnvData
{
    RenderResourceHandle envData;
};

struct SSAOData
{
    RenderResourceHandle ssaoFBO;
    RenderResourceHandle ssaoBlurFBO;
    RenderResourceHandle ssaoNoiseTexData;
    RenderResourceHandle ssaoIntermediateTexData;
};

struct VXGIData
{
    RenderResourceHandle voxelBuildFBO;
    RenderResourceHandle albedoData;
    RenderResourceHandle normalData;
    RenderResourceHandle radianceData;
};

struct PathTracingData
{
    RenderResourceHandle pathTracingFBO;
    RenderResourceHandle pathTracingTexData;
    RenderResourceHandle pathTracingAccumTexData;

    RenderResourceHandle vertexData;
    RenderResourceHandle indiceData;
    RenderResourceHandle normalData;
    RenderResourceHandle uvData;
    RenderResourceHandle bvhData;
};

