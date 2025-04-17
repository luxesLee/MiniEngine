#pragma once

class FrameGraph;
class FrameGraphBlackboard;
class Scene;

class GBufferPass
{
public:
    GBufferPass()
    {
    }
    ~GBufferPass()
    {
    }

    void AddPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);
    void Init();

private:
    void AddPreDepthPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);
};
