#pragma once

class FrameGraph;
class FrameGraphBlackboard;
class Scene;

class DeferredLightingPass
{
public:
    DeferredLightingPass() {}
    ~DeferredLightingPass() {}
    
    void AddPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);
};