#pragma once
#include "Core/Scene.h"

class FrameGraph;
class FrameGraphBlackboard;

class GBufferPass
{
public:
    GBufferPass()
    {
    }
    ~GBufferPass()
    {
    }

    void AddPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene, RenderResource& renderResource);
    void Init();

private:
    void AddPreDepthPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);
};
