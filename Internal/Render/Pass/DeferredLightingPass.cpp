#include "DeferredLightingPass.h"
#include "Render/ShaderManager.h"
#include "Core/Config.h"
#include "Core/Shader.h"
#include "Render/RenderResource.h"

#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"

void DeferredLightingPass::AddPass(FrameGraph &fg, FrameGraphBlackboard &blackboard, Scene *scene, RenderResource& renderResource)
{
    Shader* shaderDeferredLighting = g_ShaderManager.GetShader("DeferredLighting");
    if(!shaderDeferredLighting)
    {
        return;
    }

    const auto gBufferData = renderResource.get<GBufferData>();

    // 输入
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBufferData.gBuffer0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBufferData.gBuffer1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gBufferData.gBuffer2);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gBufferData.gBuffer3);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, gBufferData.gBufferDepthStencil);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_BUFFER, scene->getLightTexId());

    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_CUBE_MAP, scene->getIrradianceEnvTexId());

    // 输出
    const auto defaultData = renderResource.get<DefaultData>();
    GLuint curOutputTexId = defaultData.defaultColorData[scene->curOutputTex];
    glBindImageTexture(0, curOutputTexId, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    // Uniform
    shaderDeferredLighting->use();
    shaderDeferredLighting->setBool("bPCF", g_Config->bPCF);
    shaderDeferredLighting->setBool("bCascade", g_Config->bCascadeShadow);
    shaderDeferredLighting->setInt("cascadeLevel", g_Config->cascadeLevel);

    int pointTexIndex = 0, otherTexIndex = 0;
    for(Int i = 0; i < scene->shadowMapCaches.size(); i++)
    {
        auto& shadowMapCache = scene->shadowMapCaches[i];
        auto light = shadowMapCache.light;
        if(!light || (light->type == DIRECTIONAL_LIGHT && i > 0))
        {
            continue;
        }

        if(light->type == DIRECTIONAL_LIGHT)
        {
            if(!g_Config->bCascadeShadow)
            {
                glActiveTexture(GL_TEXTURE9);
                glBindTexture(GL_TEXTURE_2D, shadowMapCache.shadowPassDepthTexIds);
                shaderDeferredLighting->setMat4("lightMat", shadowMapCache.lightMats[0]);
            }
            else
            {
                glActiveTexture(GL_TEXTURE10);
                glBindTexture(GL_TEXTURE_2D_ARRAY, shadowMapCache.shadowPassDepthTexIds);
                for(Int cascadeIndex = 0; cascadeIndex < g_Config->cascadeLevel; cascadeIndex++)
                {
                    shaderDeferredLighting->setMat4("cascadeMat[" + std::to_string(cascadeIndex) + "]", shadowMapCache.lightMats[cascadeIndex]);
                }
            }
        }
        else if(light->type == POINT_LIGHT && pointTexIndex < 2)
        {
            glActiveTexture(GL_TEXTURE11 + pointTexIndex);
            glBindTexture(GL_TEXTURE_CUBE_MAP, shadowMapCache.shadowPassDepthTexIds);
            pointTexIndex++;
        }
        else if(otherTexIndex < 2)
        {
            glActiveTexture(GL_TEXTURE13 + otherTexIndex);
            glBindTexture(GL_TEXTURE_2D, shadowMapCache.shadowPassDepthTexIds);
            otherTexIndex++;
        }
    }


    glDispatchCompute(g_Config->wholeWidth / 32, g_Config->screenHeight / 32, 1);
}
