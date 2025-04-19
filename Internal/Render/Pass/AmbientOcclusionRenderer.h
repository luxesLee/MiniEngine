#pragma once
#include "SSAOPass.h"

class AmbientOcclusionRenderer
{
public:
    AmbientOcclusionRenderer() {}
    
    ~AmbientOcclusionRenderer() {}

    void AddPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);

private:
    SSAOPass ssaoPass;
    // todo SSBO
};
