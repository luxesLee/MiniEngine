#pragma once
#include "SSAOPass.h"

class AmbientOcclusionRenderer
{
public:
    AmbientOcclusionRenderer() {}
    
    ~AmbientOcclusionRenderer() {}

    void AddPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene, RenderResource& renderResource);

private:
    SSAOPass ssaoPass;
    // todo SSBO
};
