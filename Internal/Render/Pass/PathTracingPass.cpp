#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"
#include "PathTracingPass.h"
#include "Render/ShaderManager.h"
#include "Render/RenderInterface.h"
#include "Render/RenderResource.h"
#include "Core/Shader.h"
#include "Core/Config.h"


PathTracingPass::PathTracingPass()
{
    Init();
}

PathTracingPass::~PathTracingPass()
{

}

void PathTracingPass::AddPass(FrameGraph &fg, FrameGraphBlackboard& blackboard, Scene* scene, RenderResource& renderResource)
{
    const auto pathTracingData = renderResource.get<PathTracingData>();
    const auto defaultData = renderResource.get<DefaultData>();

    Shader* shaderPathTracing = g_ShaderManager.GetShader("PathTracing");
    if(!shaderPathTracing)
    {
        return;
    }

    glBindVertexArray(defaultData.defaultVAO);
    glBindFramebuffer(GL_FRAMEBUFFER, pathTracingData.pathTracingFBO);

    if(g_Config->accumulateFrames == 1)
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pathTracingData.pathTracingAccumTexData);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, scene->getVertTexId());
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_BUFFER, scene->getIndiceTexId());
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_BUFFER, scene->getNormalTexId());
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_BUFFER, scene->getUVTexId());
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_BUFFER, scene->getBVHTexId());
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, scene->getMatTexId());
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_BUFFER, scene->getLightTexId());
    // glActiveTexture(GL_TEXTURE8);
    // glBindTexture(GL_TEXTURE_BUFFER, scene->getEnvTexId());
    // checkGLError();
    glActiveTexture(GL_TEXTURE9);
    glBindTexture(GL_TEXTURE_2D, scene->getTransformTexId());
    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D_ARRAY, scene->getTextureArrayId());

    shaderPathTracing->use();
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, pathTracingData.pathTracingFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, defaultData.defaultFBO);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBlitFramebuffer(0, 0, g_Config->screenWidth, g_Config->screenHeight, 0, 0, g_Config->screenWidth, g_Config->screenHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void PathTracingPass::Init()
{

}
