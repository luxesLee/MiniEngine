#include "ToneMappingPass.h"
#include "Render/ShaderManager.h"
#include "Core/Shader.h"
#include "core/Camera.h"
#include "core/Config.h"
#include "Render/RenderResource.h"
#include <fg/FrameGraph.hpp>
#include <fg/Blackboard.hpp>

ToneMappingPass::ToneMappingPass()
{
}

ToneMappingPass::~ToneMappingPass()
{
}

void ToneMappingPass::AddPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene, RenderResource& renderResource)
{
    const auto defaultData = renderResource.get<DefaultData>();
    GLuint curOutputTexId = defaultData.defaultColorData[scene->curOutputTex];

    Shader* shaderToneMapping = g_ShaderManager.GetShader("ToneMapping");
    if(!shaderToneMapping)
    {
        return;
    }

    glBindImageTexture(0, curOutputTexId, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    shaderToneMapping->use();
    shaderToneMapping->setInt("toneMappingType", Int(g_Config->curToneMapping));

    glDispatchCompute(g_Config->wholeWidth / 32, g_Config->screenHeight / 32, 1);
}

void ToneMappingPass::Init()
{
}
