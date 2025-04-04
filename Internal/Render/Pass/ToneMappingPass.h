#pragma once
#include <cstdint>

class FrameGraph;

class ToneMappingPass
{
public:
    ToneMappingPass();
    ~ToneMappingPass();

    void AddPass(FrameGraph& fg);
    void Resize(uint32_t width, uint32_t height);
};
