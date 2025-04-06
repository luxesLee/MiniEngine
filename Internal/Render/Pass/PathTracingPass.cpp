#include "PathTracingPass.h"
#include "Render/ShaderManager.h"
#include "Core/Scene.h"
#include "Core/Shader.h"
#include "Render/RenderResHelper.h"
#include <fg/FrameGraph.hpp>
#include <fg/Blackboard.hpp>
#include "core/Camera.h"

GLuint vao;
PathTracingPass::PathTracingPass()
{
    InitFBO();
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
}

PathTracingPass::~PathTracingPass()
{
    if(pathTracingFBO)
    {
        RenderResHelper::deleteFBO();
    }

    if(pathTracingTexId)
    {
        glDeleteTextures(1, &pathTracingTexId);
    }

    if(accumTexId)
    {
        glDeleteTextures(1, &accumTexId);
    }
}

void PathTracingPass::AddPass(FrameGraph &fg, FrameGraphBlackboard& blackboard, Scene* scene)
{
    Shader* shader = g_ShaderManager.GetShader("PathTracing");
    if(shader)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pathTracingFBO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, accumTexId);
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
        checkGLError();
       
        shader->use();
        checkGLError();
        glDrawArrays(GL_TRIANGLES, 0, 3);
   
        glBindFramebuffer(GL_READ_FRAMEBUFFER, pathTracingFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBlitFramebuffer(0, 0, g_Camera->screenWidth, g_Camera->screenHeight, 0, 0, g_Camera->screenWidth, g_Camera->screenHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
}

void PathTracingPass::Resize(uint32_t width, uint32_t height)
{
    if(pathTracingFBO)
    {
        //RenderResHelper::deleteFBO();
        glDeleteFramebuffers(1, &pathTracingFBO);
    }

    if(pathTracingTexId)
    {
        glDeleteTextures(1, &pathTracingTexId);
    }

    if(accumTexId)
    {
        glDeleteTextures(1, &accumTexId);
    }

    InitFBO();
}

void PathTracingPass::InitFBO()
{
    // temporarily write
    glGenFramebuffers(1, &pathTracingFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, pathTracingFBO);

    glGenTextures(1, &pathTracingTexId);
    glBindTexture(GL_TEXTURE_2D, pathTracingTexId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, g_Camera->screenWidth, g_Camera->screenHeight, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pathTracingTexId, 0);

    glGenTextures(1, &accumTexId);
    glBindTexture(GL_TEXTURE_2D, accumTexId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, g_Camera->screenWidth, g_Camera->screenHeight, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, accumTexId, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
