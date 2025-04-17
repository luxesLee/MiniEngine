#include "DeferredLightingPass.h"
#include "Render/ShaderManager.h"
#include "Core/Config.h"
#include "Core/Shader.h"
#include "Core/Scene.h"

#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"

void DeferredLightingPass::AddPass(FrameGraph &fg, FrameGraphBlackboard &blackboard, Scene *scene)
{
    Shader* shaderDeferredLighting = g_ShaderManager.GetShader("DeferredLighting");
    if(!shaderDeferredLighting)
    {
        return;
    }

    // 输入
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scene->GBufferTexId[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, scene->GBufferTexId[1]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, scene->GBufferTexId[2]);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, scene->GBufferTexId[3]);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, scene->basePassDepthStencilTexId);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_BUFFER, scene->getLightTexId());
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, scene->LTC1Tex.texId);
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, scene->LTC2Tex.texId);

    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, scene->shadowPassDepthTexIds[0]);

    // 输出
    glBindImageTexture(0, scene->outputTex[scene->curOutputTex], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    shaderDeferredLighting->use();

    shaderDeferredLighting->setMat4("lightMat", scene->lightMats[0]);

    glDispatchCompute(g_Config->wholeWidth / 32, g_Config->screenHeight / 32, 1);
}
