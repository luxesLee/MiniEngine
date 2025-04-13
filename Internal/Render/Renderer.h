#pragma once
#include "Pass/BasePass.h"
#include "Pass/PathTracingPass.h"
#include "Pass/ToneMappingPass.h"
#include "Pass/IDenoisePass.h"

class FrameGraph;
class FrameGraphBlackboard;
class Scene;



class Renderer
{
public:
    Renderer();
    ~Renderer() 
    {
        if(denoisePass)
        {
            delete denoisePass;
        }
    }

    void Update();
    void Render(Scene* scene);
    void Resize();

private:
    void UpdateUBO();
    void DoCulling();
    void ForwardRendering(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);
    void DeferredRendering(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);
    void PathTracing(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);


private:
    BasePass        basePass;

    PathTracingPass pathTracingPass;
    ToneMappingPass toneMappingPass;
    IDenoisePass* denoisePass = nullptr;

    GLuint CommonUBO;
    GLuint PathTracingUBO;
};