#include "Renderer.h"
#include "Core/Scene.h"
#include "ShaderManager.h"
#include "core/Camera.h"
#include "RenderDebug.h"
#include "Core/Config.h"
#include <fg/FrameGraph.hpp>
#include <fg/Blackboard.hpp>
#include "RenderResource.h"

Renderer::Renderer()
{
    g_ShaderManager.InitShader();

    if(g_Config->curDenoise != DenoiseType::NONE)
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
        vxgiPass.AddBuildPass(fg, blackboard, scene, renderResource);
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
    if(g_Config->debugMode != DebugMode::NONE)
    {
        DebugRendering(fg, blackboard, scene);
    }
}

void Renderer::DebugRendering(FrameGraph &fg, FrameGraphBlackboard &blackboard, Scene *scene)
{
    const auto defaultData = renderResource.get<DefaultData>();
    defaultData.defaultFBO;
    glBindFramebuffer(GL_FRAMEBUFFER, scene->outputFBO);
    switch (g_Config->debugMode)
    {
    case DebugMode::DebugShadow:
        glBindFramebuffer(GL_FRAMEBUFFER, scene->outputFBO);
        shadowRenderer.AddPassVisualizeShadowMap(scene);
        break;
    case DebugMode::DebugVXGI:
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
    if(g_Config->curToneMapping != ToneMappingType::NONE)
    {
        toneMappingPass.AddPass(fg, blackboard, scene);
    }

    if(denoisePass)
    {
        denoisePass->AddPass(scene);
    }
}

void Renderer::InitRenderResource()
{
    GLuint defaultFBO = generateFBO();

    TextureDesc screenTexDesc{g_Config->wholeWidth, g_Config->screenHeight, 0, TextureType1::TEXTURE_2D, TextureFormat::RGBA32F,
                            LINEAR_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 0, nullptr, DataFormat::RGBA, DataType::FLOAT};
    GPUTexture outputTex0 = generateTexture(screenTexDesc);
    GPUTexture outputTex1 = generateTexture(screenTexDesc);
    
    TextureDesc depthTexDesc{g_Config->wholeWidth, g_Config->screenHeight, 0, TextureType1::TEXTURE_2D, TextureFormat::DEPTH24_STENCIL8,
                            NEAREST_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 0, nullptr, DataFormat::DEPTH_STENCIL, DataType::UNSIGNED_INT_24_8};
    GPUTexture outputDepthTex = generateTexture(depthTexDesc);

    renderResource.add<DefaultData>(defaultFBO, outputTex0.texId, outputTex1.texId, outputDepthTex.texId);

    if(g_Config->lightMode == LightMode::PathTracing)
    {
        GLuint pathTracingFBO = generateFBO();

        GPUTexture pathTracingTex = generateTexture(screenTexDesc);
        GPUTexture accumTex = generateTexture(screenTexDesc);

        glBindFramebuffer(GL_FRAMEBUFFER, pathTracingFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pathTracingTex.texId, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, accumTex.texId, 0);
        GLenum DrawBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, DrawBuffers);

        renderResource.add<PathTracingData>(pathTracingFBO, pathTracingTex.texId, accumTex.texId);
    }
    else if(g_Config->lightMode == LightMode::Forward)
    {

    }
    else
    {



        renderResource.add<GBufferData>();
    }

    if(g_Config->bShadeShadow)
    {
        
    }

    if(g_Config->bVXGI)
    {
        GLuint vxgiFBO = generateFBO();

        TextureDesc desc1{g_Config->VoxelSize, g_Config->VoxelSize, g_Config->VoxelSize, TextureType1::TEXTURE_3D, TextureFormat::RGBA8,
                        LINEAR_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 1};
        GPUTexture albedo3DTex = generateTexture(desc1);
        GPUTexture normal3DTex = generateTexture(desc1);

        TextureDesc desc2{g_Config->VoxelSize, g_Config->VoxelSize, g_Config->VoxelSize, TextureType1::TEXTURE_3D, TextureFormat::RGBA8,
                LINEAR_MIPMAP_LINEAR_LINEAR_CLAMP_TO_BORDER_BLACK_BORDER_SAMPLER, Int(std::log2(g_Config->VoxelSize / 8))};
        GPUTexture radiance3DTex = generateTexture(desc2);

        renderResource.add<VXGIData>(vxgiFBO, albedo3DTex.texId, normal3DTex.texId, radiance3DTex.texId);
    }



}
