#pragma once
#include <cstdint>
#include "glad/glad.h"
#include "Core/Scene.h"

class FrameGraph;
class FrameGraphBlackboard;
class Scene;

class ToneMappingPass
{
public:
    ToneMappingPass();
    ~ToneMappingPass();

    void AddPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene, RenderResource& renderResource);
    void Init();
};
