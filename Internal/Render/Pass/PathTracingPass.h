#pragma once
#include "glad/glad.h"
#include <cstdint>

class FrameGraph;
class FrameGraphBlackboard;
class Scene;

class PathTracingPass
{
public:
    PathTracingPass();
    ~PathTracingPass();

    void AddPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);
    void Init();

private:
    void InitFBO();

private:
    GLuint pathTracingFBO;
    GLuint pathTracingTexId;

    GLuint accumFBO;
    GLuint accumTexId;
};
