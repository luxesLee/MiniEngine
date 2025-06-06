#include "GBufferPass.h"
#include "Render/ShaderManager.h"
#include "Core/Config.h"
#include "Core/Shader.h"
#include "Render/RenderResource.h"

#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"
#include "Render/RenderResource.h"

void GBufferPass::AddPass(FrameGraph &fg, FrameGraphBlackboard &blackboard, Scene *scene, RenderResource& renderResource)
{
    const auto gBufferData = renderResource.get<GBufferData>();
    const auto gpuMatData = renderResource.get<GPUMaterialData>();
    const auto gpuTransformData = renderResource.get<GPUTransformData>();
    const auto gpuCombinedTexData = renderResource.get<GPUCombinedTextureData>();

    glBindFramebuffer(GL_FRAMEBUFFER, gBufferData.gBufferFBO);

    Shader* shaderBasePass = g_ShaderManager.GetShader("BasePass");
    if(!shaderBasePass)
    {
        return;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(g_Config->bPreDepthPass)
    {
        AddPreDepthPass(fg, blackboard, scene);
    }

    shaderBasePass->use();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(g_Config->bPreDepthPass ? GL_EQUAL : GL_GREATER);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gpuTransformData.transformData);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gpuMatData.matTexData);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D_ARRAY, gpuCombinedTexData.combinedTexData);

    auto meshBatches = scene->GetMeshBatches();
    Int instanceBase = 0, curInstanceCount = 0;
    for(auto& meshBatch : meshBatches)
    {
        curInstanceCount = meshBatch->GetInstanceCount();
        shaderBasePass->setInt("instanceBase", instanceBase);
        meshBatch->Bind();
        if(meshBatch->GetDrawElement())
        {
            glDrawElementsInstanced(GL_TRIANGLES, meshBatch->GetNumPerMeshBatch(), GL_UNSIGNED_INT, 0, curInstanceCount);
        }
        else
        {
            glDrawArraysInstanced(GL_TRIANGLES, 0, meshBatch->GetNumVertices(), curInstanceCount);
        }
        meshBatch->UnBind();
        instanceBase += curInstanceCount;
    }

    // glBindFramebuffer(GL_READ_FRAMEBUFFER, scene->deferredBasePassFBO);
    // glBindFramebuffer(GL_DRAW_FRAMEBUFFER, scene->outputFBO);
    // glBlitFramebuffer(0, 0, g_Config->screenWidth, g_Config->screenHeight, 0, 0, g_Config->screenWidth, g_Config->screenHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void GBufferPass::AddPreDepthPass(FrameGraph &fg, FrameGraphBlackboard &blackboard, Scene *scene)
{
    Shader* shaderPreDepthPass = g_ShaderManager.GetShader("PreDepthPass");
    if(!shaderPreDepthPass)
    {
        return;
    }

    shaderPreDepthPass->use();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);


}
