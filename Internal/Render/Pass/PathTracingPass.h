#pragma once
#include "glad/glad.h"
#include "Core/Scene.h"
#include <cstdint>

class FrameGraph;
class FrameGraphBlackboard;
class Scene;

class PathTracingPass
{
public:
    PathTracingPass();
    ~PathTracingPass();

    void AddPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene, RenderResource& renderResource);
    void Init();

private:
    GLuint vao;
};
