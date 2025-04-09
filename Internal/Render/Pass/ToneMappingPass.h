#pragma once
#include <cstdint>
#include "glad/glad.h"

class FrameGraph;
class FrameGraphBlackboard;
class Scene;

class ToneMappingPass
{
public:
    ToneMappingPass();
    ~ToneMappingPass();

    void AddPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);
    void Init();
};
