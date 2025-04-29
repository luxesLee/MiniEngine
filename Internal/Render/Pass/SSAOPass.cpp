#include "SSAOPass.h"
#include "Render/ShaderManager.h"
#include "Core/Shader.h"
#include "Core/Camera.h"
#include "Render/RenderResource.h"

#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"

#include <random>


SSAOPass::SSAOPass()
{
    auto lerp = [](float a, float b, float f)
    {
        return a + f * (b - a);
    };

    std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    for (unsigned int i = 0; i < g_Config->SSAOKernelSize; ++i)
    {
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = float(i) / g_Config->SSAOKernelSize;

        // scale samples s.t. they're more aligned to center of kernel
        scale = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
}

SSAOPass::~SSAOPass()
{
    glDeleteVertexArrays(1, &vao);
}

void SSAOPass::AddPass(FrameGraph &fg, FrameGraphBlackboard &blackboard, Scene *scene, RenderResource& renderResource)
{
    const auto gBufferData = renderResource.get<GBufferData>();
    gBufferTexId[0] = gBufferData.gBuffer0;
    gBufferTexId[1] = gBufferData.gBuffer1;

    const auto ssaoData = renderResource.get<SSAOData>();
    ssaoFBO = ssaoData.ssaoFBO;
    ssaoBlurFBO = ssaoData.ssaoBlurFBO;
    ssaoNoisyTexId = ssaoData.ssaoNoiseTexData;
    ssaoIntermediateTexId = ssaoData.ssaoIntermediateTexData;

    AddSSAOPass(fg, blackboard, scene);
    AddSSAOBlurPass(fg, blackboard, scene);
}

void SSAOPass::AddSSAOPass(FrameGraph &fg, FrameGraphBlackboard &blackboard, Scene *scene)
{
    Shader* shaderSSAO = g_ShaderManager.GetShader("SSAO");
    if(!shaderSSAO)
    {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBufferTexId[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBufferTexId[1]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, ssaoNoisyTexId);

    shaderSSAO->use();
    shaderSSAO->setInt("ssaoKernel", g_Config->SSAOKernelSize);
    shaderSSAO->setFloat("ssaoRadius", g_Config->SSAORadius);
    shaderSSAO->setFloat("ssaoBias", g_Config->SSAOBias);
    for(Int i = 0; i < g_Config->SSAOKernelSize; i++)
    {
        shaderSSAO->setVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
    }
    glBindVertexArray(vao);

    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void SSAOPass::AddSSAOBlurPass(FrameGraph &fg, FrameGraphBlackboard &blackboard, Scene *scene)
{
    Shader* shaderSSAOBlur = g_ShaderManager.GetShader("SSAOBlur");
    if(!shaderSSAOBlur)
    {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gBufferTexId[0], 0);

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssaoIntermediateTexId);

    shaderSSAOBlur->use();

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
