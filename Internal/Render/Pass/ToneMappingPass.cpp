#include "ToneMappingPass.h"
#include "Render/ShaderManager.h"
#include "Core/Scene.h"
#include "Core/Shader.h"
#include "core/Camera.h"
#include "core/Config.h"
#include <fg/FrameGraph.hpp>
#include <fg/Blackboard.hpp>

ToneMappingPass::ToneMappingPass()
{
}

ToneMappingPass::~ToneMappingPass()
{
}

void ToneMappingPass::AddPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene)
{
    Shader* shaderToneMapping = g_ShaderManager.GetShader("ToneMapping");
    if(!shaderToneMapping)
    {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, scene->toneMappingFBO);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scene->pathTracingTexId);

    shaderToneMapping->use();
    shaderToneMapping->setInt("toneMappingType", g_Config->curToneMapping);

    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    glBindFramebuffer(GL_READ_FRAMEBUFFER, scene->toneMappingFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, scene->outputFBO);
    glBlitFramebuffer(0, 0, g_Camera->screenWidth, g_Camera->screenHeight, 0, 0, g_Camera->screenWidth, g_Camera->screenHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void ToneMappingPass::Init()
{
}
