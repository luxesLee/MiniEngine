#pragma once
#include "Core/Scene.h"

class FrameGraph;
class FrameGraphBlackboard;

class DeferredLightingPass
{
public:
    DeferredLightingPass() {}
    ~DeferredLightingPass() {}
    
    void AddPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene, RenderResource& renderResource);
};