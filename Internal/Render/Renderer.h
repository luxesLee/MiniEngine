#pragma once
#include "Pass/PathTracingPass.h"
#include "Pass/ToneMappingPass.h"
#include "Pass/IDenoisePass.h"

class FrameGraph;
class FrameGraphBlackboard;
class Scene;



class Renderer
{
public:
    Renderer() {}
    Renderer(uint32_t width, uint32_t height);
    ~Renderer() {}

    void Update();
    void Render(Scene* scene);
    void Resize(uint32_t width, uint32_t height);

private:
    void UpdateUBO();
    void DoCulling();
    void ForwardRendering();
    void DeferredRendering();
    void PathTracing(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);


private:
    PathTracingPass pathTracingPass;
    ToneMappingPass toneMappingPass;
    IDenoisePass* denoisePass = nullptr;

    GLuint CommonUBO;
    GLuint PathTracingUBO;
};