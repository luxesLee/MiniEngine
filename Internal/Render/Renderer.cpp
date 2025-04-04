#include "Renderer.h"
#include "Core/Scene.h"
#include "ShaderManager.h"
#include "core/Camera.h"
#include "RenderDebug.h"
#include "Core/Config.h"
#include <fg/FrameGraph.hpp>
#include <fg/Blackboard.hpp>

Renderer::Renderer(uint32_t width, uint32_t height)
{
    g_ShaderManager.InitShader();
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
}

void Renderer::Render(Scene *scene)
{
    if(g_Config->bRenderdocCapture)
    {
        RenderDebugger::startFrameCapture();
    }

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

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

void Renderer::Resize(uint32_t width, uint32_t height)
{
    glViewport(0, 0, width, height);
    pathTracingPass.Resize(width, height);
    toneMappingPass.Resize(width, height);
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
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &g_Config->maxRayTracingDepth);
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(int), sizeof(int), &g_Config->accumulateFrames);
        glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(int), sizeof(int), &topBVHIndex);
        glBufferSubData(GL_UNIFORM_BUFFER, 3 * sizeof(int), sizeof(int), &maxDepth);
    }

    pathTracingPass.AddPass(fg, blackboard, scene);
    // toneMappingPass.AddPass(fg);
}
