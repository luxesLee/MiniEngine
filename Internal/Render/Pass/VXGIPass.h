#pragma once

class FrameGraph;
class FrameGraphBlackboard;
class Scene;

class VXGIPass
{
public:
    VXGIPass() {}

    ~VXGIPass() {}

    void AddPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);

private:
    void AddVoxelSceneBuildPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);

    void AddLightInjectPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);
    
    void AddIndirectLightingPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);
};
