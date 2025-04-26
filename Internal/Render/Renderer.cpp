#include "Renderer.h"
#include "Core/Scene.h"
#include "ShaderManager.h"
#include "core/Camera.h"
#include "RenderDebug.h"
#include "Core/Config.h"
#include <fg/FrameGraph.hpp>
#include <fg/Blackboard.hpp>

Renderer::Renderer()
{
    g_ShaderManager.InitShader();

    if(g_Config->curDenoise > 0)
    {
        denoisePass = CreateDenoisePass();
    }

    glViewport(0, 0, g_Config->screenWidth, g_Config->screenHeight);
    glGenBuffers(1, &CommonUBO);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, CommonUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, CommonUBO);
    glBufferData(GL_UNIFORM_BUFFER, 6 * sizeof(glm::mat4) + 2 * sizeof(glm::vec4) + sizeof(Int), NULL, GL_DYNAMIC_DRAW);
    glGenBuffers(1, &PathTracingUBO);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, PathTracingUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, PathTracingUBO);
    glBufferData(GL_UNIFORM_BUFFER, 3 * sizeof(int), NULL, GL_DYNAMIC_DRAW);

    glGenVertexArrays(1, &defaultVAO);
    glBindVertexArray(defaultVAO);
}

void Renderer::Update(Scene* scene)
{
    // we will update ubo and complete the culling in this function
    UpdateUBO(scene);
    DoCulling();

    if(denoisePass && g_Config->curDenoise != denoisePass->GetType())
    {
        delete denoisePass;
        denoisePass = CreateDenoisePass();
    }
    else if(!denoisePass && g_Config->curDenoise != DenoiseType::NONEDenoise)
    {
        denoisePass = CreateDenoisePass();
    }

}

void Renderer::Render(Scene *scene)
{
    if(g_Config->bRenderdocCapture)
    {
        RenderDebugger::startFrameCapture();
    }

    // if(g_Config->accumulateFrames == 0)
    // {
    //     glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    //     glClear(GL_COLOR_BUFFER_BIT);
    // }

    scene->curOutputTex = 1 - scene->curOutputTex;
    glBindFramebuffer(GL_FRAMEBUFFER, scene->outputFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, scene->outputTex[scene->curOutputTex], 0);

    FrameGraph fg;
    FrameGraphBlackboard blackboard;
    // todo: forward and deferred
    switch (g_Config->lightMode)
    {
    case LightMode::Forward: ForwardRendering(fg, blackboard, scene); break;
    case LightMode::Deferred:
    case LightMode::TiledDeferred:
    case LightMode::ClusterDeferred:
        DeferredRendering(fg, blackboard, scene);
        break;
    case LightMode::PathTracing: PathTracing(fg, blackboard, scene); break;
    }

    // todo: rendergraph
    // fg.compile();
    // fg.execute();

    glBindFramebuffer(GL_FRAMEBUFFER, scene->outputFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, scene->outputTex[1 - scene->curOutputTex], 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, scene->outputFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, g_Config->wholeWidth, g_Config->screenHeight, 0, 0, g_Config->wholeWidth, g_Config->screenHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);


    if(g_Config->bRenderdocCapture)
    {
        g_Config->bRenderdocCapture = false;
        RenderDebugger::endFrameCapture();
    }
}

void Renderer::Resize()
{
    glViewport(0, 0, g_Config->screenWidth, g_Config->screenHeight);
}

void Renderer::UpdateUBO(Scene* scene)
{
    if(CommonUBO)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, CommonUBO);
        // Model
        glm::mat4 indentity = glm::mat4(1.0f);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &indentity);
        // View
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &g_Camera->GetViewMatrix());
        // Projection
        glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), sizeof(glm::mat4), &g_Camera->GetProjectionMatrix());
        // ProjectionView
        glBufferSubData(GL_UNIFORM_BUFFER, 3 * sizeof(glm::mat4), sizeof(glm::mat4), &(g_Camera->GetProjectionMatrix() * g_Camera->GetViewMatrix()));
        // invProjection
        glBufferSubData(GL_UNIFORM_BUFFER, 4 * sizeof(glm::mat4), sizeof(glm::mat4), &glm::inverse(g_Camera->GetProjectionMatrix()));
        // invViewProjection
        glBufferSubData(GL_UNIFORM_BUFFER, 5 * sizeof(glm::mat4), sizeof(glm::mat4), &glm::inverse(g_Camera->GetProjectionMatrix() * g_Camera->GetViewMatrix()));
        // screenAndInvScreen
        glBufferSubData(GL_UNIFORM_BUFFER, 6 * sizeof(glm::mat4), sizeof(glm::vec4), &g_Camera->GetScreenAndInvScreen());
        // cameraPosition
        glBufferSubData(GL_UNIFORM_BUFFER, 6 * sizeof(glm::mat4) + sizeof(glm::vec4), sizeof(glm::vec4), &g_Camera->Position);
        // lightNum
        int lightNum = scene->getLightNum();
        glBufferSubData(GL_UNIFORM_BUFFER, 6 * sizeof(glm::mat4) + 2 * sizeof(glm::vec4), sizeof(Int), &lightNum);
    }

    if(PathTracingUBO)
    {
        g_Config->accumulateFrames++;
        glBindBuffer(GL_UNIFORM_BUFFER, PathTracingUBO);
        int maxDepth = 1;
        int topBVHIndex = scene->getTopBVHIndex();
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Int), &g_Config->maxRayTracingDepth);
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(Int), sizeof(Int), &g_Config->accumulateFrames);
        glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(Int), sizeof(Int), &topBVHIndex);
    }
}

void Renderer::DoCulling()
{
}

void Renderer::ForwardRendering(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene)
{
    
}

void Renderer::DeferredRendering(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene)
{
    // BasePass
    basePass.AddPass(fg, blackboard, scene);

    if(g_Config->bSSAO)
    {
        aoRenderer.AddPass(fg, blackboard, scene);
    }

    // ShadowMap
    if(g_Config->bShadeShadow || g_Config->bVXGI)
    {
        glViewport(0, 0, g_Config->shadowDepthWidth, g_Config->shadowDepthHeight);
        shadowRenderer.AddPass(fg, blackboard, scene);
        glViewport(0, 0, g_Config->screenWidth, g_Config->screenHeight);
    }

    // VXGI
    if(g_Config->bVXGI)
    {
        glViewport(0, 0, g_Config->VoxelSize, g_Config->VoxelSize);
        vxgiPass.AddBuildPass(fg, blackboard, scene);
        glViewport(0, 0, g_Config->screenWidth, g_Config->screenHeight);
    }

    // Direct Lighting
    switch (g_Config->lightMode)
    {
    case LightMode::Deferred: deferredLightingPass.AddPass(fg, blackboard, scene); break;
    case LightMode::TiledDeferred: break;
    case LightMode::ClusterDeferred: break;
    }

    if(g_Config->bVXGI)
    {
        vxgiPass.AddIndirectLightingPass(fg, blackboard, scene);
    }

    if(g_Config->bSSR)
    {

    }

    // Post Process
    postProcessPass.AddPass(fg, blackboard, scene);

    // Debug
    if(g_Config->debugMode != NONEDebug)
    {
        DebugRendering(fg, blackboard, scene);
    }
}

void Renderer::DebugRendering(FrameGraph &fg, FrameGraphBlackboard &blackboard, Scene *scene)
{
    glBindFramebuffer(GL_FRAMEBUFFER, scene->outputFBO);
    switch (g_Config->debugMode)
    {
    case DebugShadow:
        glBindFramebuffer(GL_FRAMEBUFFER, scene->outputFBO);
        shadowRenderer.AddPassVisualizeShadowMap(scene);
        break;
    case DebugVXGI:
        glBindVertexArray(defaultVAO);
        vxgiPass.AddDebugPass(fg, blackboard, scene);
    default:
        break;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::PathTracing(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene)
{
    pathTracingPass.AddPass(fg, blackboard, scene);
    if(g_Config->curToneMapping > 0)
    {
        toneMappingPass.AddPass(fg, blackboard, scene);
    }

    if(denoisePass)
    {
        denoisePass->AddPass(scene);
    }
}
