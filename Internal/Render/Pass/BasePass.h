#pragma once

class FrameGraph;
class FrameGraphBlackboard;
class Scene;

class BasePass
{
public:
    BasePass()
    {
    }
    ~BasePass()
    {
    }

    void AddPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);
    void AddPreDepthPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);

    void Init();
};
