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

    glViewport(g_Config->imguiWidth, 0, g_Config->wholeWidth, g_Config->screenHeight);
    glGenBuffers(1, &CommonUBO);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, CommonUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, CommonUBO);
    glBufferData(GL_UNIFORM_BUFFER, 5 * sizeof(glm::mat4) + 2 * sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);
    glGenBuffers(1, &PathTracingUBO);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, PathTracingUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, PathTracingUBO);
    glBufferData(GL_UNIFORM_BUFFER, 4 * sizeof(int), NULL, GL_DYNAMIC_DRAW);
}

void Renderer::Update()
{
    // we will update ubo and complete the culling in this function
    UpdateUBO();
    DoCulling();

    if(denoisePass && g_Config->curDenoise != denoisePass->GetType())
    {
        delete denoisePass;
        denoisePass = CreateDenoisePass();
    }
    else if(!denoisePass && g_Config->curDenoise != DenoiseType::NONE)
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

    FrameGraph fg;
    FrameGraphBlackboard blackboard;
    // todo: forward and deferred
    switch (g_Config->lightMode)
    {
    case LightMode::Forward:
        ForwardRendering();
        break;
    case LightMode::Deferred:
    case LightMode::TiledDeferred:
    case LightMode::ClusterDeferred:
        DeferredRendering();
        break;
    case LightMode::PathTracing:
        PathTracing(fg, blackboard, scene);
        break;
    }

    // todo: rendergraph
    // fg.compile();
    // fg.execute();



    if(g_Config->bRenderdocCapture)
    {
        g_Config->bRenderdocCapture = false;
        RenderDebugger::endFrameCapture();
    }
}

void Renderer::Resize()
{
    glViewport(g_Config->imguiWidth, 0, g_Config->wholeWidth, g_Config->screenHeight);
}

void Renderer::UpdateUBO()
{
    if(CommonUBO)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, CommonUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &g_Camera->GetViewMatrix());
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &g_Camera->GetViewMatrix());
        glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), sizeof(glm::mat4), &g_Camera->GetProjectionMatrix());
        glBufferSubData(GL_UNIFORM_BUFFER, 3 * sizeof(glm::mat4), sizeof(glm::mat4), &glm::inverse(g_Camera->GetProjectionMatrix()));
        glBufferSubData(GL_UNIFORM_BUFFER, 4 * sizeof(glm::mat4), sizeof(glm::mat4), &glm::inverse(g_Camera->GetProjectionMatrix() * g_Camera->GetViewMatrix()));
        glBufferSubData(GL_UNIFORM_BUFFER, 5 * sizeof(glm::mat4), sizeof(glm::vec4), &g_Camera->GetScreenAndInvScreen());
        glBufferSubData(GL_UNIFORM_BUFFER, 5 * sizeof(glm::mat4) + sizeof(glm::vec4), sizeof(glm::vec4), &g_Camera->Position);
    }

}

void Renderer::DoCulling()
{
}

void Renderer::ForwardRendering()
{
    
}

void Renderer::DeferredRendering()
{
}

void Renderer::PathTracing(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene)
{
    if(PathTracingUBO)
    {
        g_Config->accumulateFrames++;
        glBindBuffer(GL_UNIFORM_BUFFER, PathTracingUBO);
        int maxDepth = 1;
        int topBVHIndex = scene->getTopBVHIndex();
        int lightNum = scene->getLightNum();
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &g_Config->maxRayTracingDepth);
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(int), sizeof(int), &g_Config->accumulateFrames);
        glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(int), sizeof(int), &topBVHIndex);
        glBufferSubData(GL_UNIFORM_BUFFER, 3 * sizeof(int), sizeof(int), &lightNum);
    }

    scene->curOutputTex = 1 - scene->curOutputTex;

    glBindFramebuffer(GL_FRAMEBUFFER, scene->outputFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, scene->outputTex[scene->curOutputTex], 0);

    pathTracingPass.AddPass(fg, blackboard, scene);
    if(g_Config->curToneMapping > 0)
    {
        toneMappingPass.AddPass(fg, blackboard, scene);
    }

    if(denoisePass)
    {
        denoisePass->AddPass(scene);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, scene->outputFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, scene->outputTex[1 - scene->curOutputTex], 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, scene->outputFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, g_Config->wholeWidth, g_Config->screenHeight, 0, 0, g_Config->wholeWidth, g_Config->screenHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}
