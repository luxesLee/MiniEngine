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
    void Resize(uint32_t width, uint32_t height);

private:
    void InitFBO();

private:
    GLuint pathTracingFBO;
    GLuint pathTracingTexId;
    GLuint accumTexId;

    
};
