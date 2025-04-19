#pragma once
#include "Pass/GBufferPass.h"
#include "Pass/AmbientOcclusionRenderer.h"
#include "Pass/ShadowRenderer.h"
#include "Pass/DeferredLightingPass.h"
#include "Pass/VXGIPass.h"
#include "Pass/PostProcessPass.h"
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

    void Update(Scene* scene);
    void Render(Scene* scene);
    void Resize();

private:
    void UpdateUBO(Scene* scene);
    void DoCulling();
    void ForwardRendering(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);
    void DeferredRendering(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);
    void PathTracing(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);


private:
    GBufferPass                 basePass;
    AmbientOcclusionRenderer    aoRenderer;
    ShadowRenderer              shadowRenderer;
    DeferredLightingPass        deferredLightingPass;
    VXGIPass                    vxgiPass;
    PostProcessPass             postProcessPass;

    PathTracingPass             pathTracingPass;
    ToneMappingPass             toneMappingPass;
    IDenoisePass*               denoisePass = nullptr;

    GLuint CommonUBO;
    GLuint PathTracingUBO;
};