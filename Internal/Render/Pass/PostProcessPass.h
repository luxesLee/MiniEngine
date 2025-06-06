#pragma once

class FrameGraph;
class FrameGraphBlackboard;
class Scene;

class PostProcessPass
{
public:
    PostProcessPass()
    {
    }
    ~PostProcessPass()
    {
    }

    void AddPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);

    void Init();
};