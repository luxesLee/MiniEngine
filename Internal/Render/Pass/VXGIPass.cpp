#include "VXGIPass.h"
#include "Render/ShaderManager.h"
#include "Core/Shader.h"
#include "Core/Camera.h"
#include "Core/Scene.h"

#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <gtx/component_wise.hpp>

void VXGIPass::Init()
{
    glGenFramebuffers(1, &voxelSceneFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, voxelSceneFBO);

    glGenTextures(1, &albedo3DTexId);
    glBindTexture(GL_TEXTURE_3D, albedo3DTexId);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA8, g_Config->VoxelSize, g_Config->VoxelSize, g_Config->VoxelSize);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glGenTextures(1, &normal3DTexId);
    glBindTexture(GL_TEXTURE_3D, normal3DTexId);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA8, g_Config->VoxelSize, g_Config->VoxelSize, g_Config->VoxelSize);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glGenTextures(1, &radiance3DTexId);
    glBindTexture(GL_TEXTURE_3D, radiance3DTexId);
    glTexStorage3D(GL_TEXTURE_3D, 8, GL_RGBA8, g_Config->VoxelSize, g_Config->VoxelSize, g_Config->VoxelSize);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, radiance3DTexId, 0, 0);
}

void VXGIPass::Delete()
{
    glDeleteFramebuffers(1, &voxelSceneFBO);
    glDeleteTextures(1, &albedo3DTexId);
    glDeleteTextures(1, &normal3DTexId);
    glDeleteTextures(1, &radiance3DTexId);
}

void VXGIPass::AddBuildPass(FrameGraph &fg, FrameGraphBlackboard &blackboard, Scene *scene)
{
    AddVoxelSceneBuildPass(fg, blackboard, scene);
    AddLightInjectPass(fg, blackboard, scene);
    AddGenerateMipmapPass(fg, blackboard, scene);
}

void VXGIPass::AddIndirectLightingPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene)
{

}

void VXGIPass::AddDebugPass(FrameGraph &fg, FrameGraphBlackboard &blackboard, Scene *scene)
{
    // FBO VAO 已在外层绑定
    Shader* shaderVisualizeVoxel = g_ShaderManager.GetShader("VisualizeVoxel");
    if(!shaderVisualizeVoxel)
    {
        return;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, radiance3DTexId);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, scene->GBufferTexId[0]);

    shaderVisualizeVoxel->use();
    shaderVisualizeVoxel->setInt("VoxelSize", g_Config->VoxelSize);
    auto bounds = scene->GetSceneBoundingBox();
    auto center = bounds.center();
    float maxCoord = glm::compMax(glm::abs(center - bounds.pmax)) * 1.1;
    projMat = glm::ortho(-maxCoord, maxCoord, -maxCoord, maxCoord, 0.001f, 2 * maxCoord + 0.001f)
                     * glm::lookAt(center - glm::vec3(0, 0, maxCoord), center, glm::vec3(0, 1, 0));
    shaderVisualizeVoxel->setMat4("VoxelProjection", projMat);

    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void VXGIPass::AddVoxelSceneBuildPass(FrameGraph &fg, FrameGraphBlackboard &blackboard, Scene *scene)
{
    Shader* shaderVoxelScene = g_ShaderManager.GetShader("VoxelScene");
    if(!shaderVoxelScene)
    {
        return;
    }
    auto meshBatch = scene->GetMeshBatches();

    auto DrawMesh = [&](Shader* shader, const std::vector<MeshBatch*>& meshBatches)
    {
        Int instanceBase = 0, curInstanceCount = 0;
        for(auto& meshBatch : meshBatches)
        {
            curInstanceCount = meshBatch->GetInstanceCount();
            shader->setInt("instanceBase", instanceBase);

            meshBatch->Bind();
            checkGLError();
            if(meshBatch->GetDrawElement())
            {
                glDrawElementsInstanced(GL_TRIANGLES, meshBatch->GetNumPerMeshBatch(), GL_UNSIGNED_INT, 0, curInstanceCount);
            }
            else
            {
                glDrawArraysInstanced(GL_TRIANGLES, 0, meshBatch->GetNumVertices(), curInstanceCount);
            }
            checkGLError();

            meshBatch->UnBind();
            instanceBase += curInstanceCount;
        }
    };

    float zero[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glClearTexImage(albedo3DTexId, 0, GL_RGBA, GL_UNSIGNED_BYTE, zero);
    glClearTexImage(normal3DTexId, 0, GL_RGBA, GL_UNSIGNED_BYTE, zero);

    glBindFramebuffer(GL_FRAMEBUFFER, voxelSceneFBO);
    glBindImageTexture(0, albedo3DTexId, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
    glBindImageTexture(1, normal3DTexId, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, scene->getMatTexId());
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D_ARRAY, scene->getTextureArrayId());

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    shaderVoxelScene->use();
    shaderVoxelScene->setInt("VoxelSize", g_Config->VoxelSize);

    auto bounds = scene->GetSceneBoundingBox();
    auto center = bounds.center();
    float maxCoord = glm::compMax(glm::abs(center - bounds.pmax)) * 1.1;
    // 正交投影用于限定场景范围，视锥即为场景包围盒
    // 视图矩阵由包围盒-z面中心指向包围盒中心，以正Y为方向
    projMat = glm::ortho(-maxCoord, maxCoord, -maxCoord, maxCoord, 0.001f, 2 * maxCoord + 0.001f)
                     * glm::lookAt(center - glm::vec3(0, 0, maxCoord), center, glm::vec3(0, 1, 0));
    shaderVoxelScene->setMat4("project", projMat);

    DrawMesh(shaderVoxelScene, meshBatch);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void VXGIPass::AddLightInjectPass(FrameGraph &fg, FrameGraphBlackboard &blackboard, Scene *scene)
{
    Shader* shaderLightInject = g_ShaderManager.GetShader("VXGILightInject");
    if(!shaderLightInject)
    {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, voxelSceneFBO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, albedo3DTexId);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, normal3DTexId);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_BUFFER, scene->getLightTexId());

    shaderLightInject->use();
    shaderLightInject->setInt("VoxelSize", g_Config->VoxelSize);
    shaderLightInject->setBool("bCascade", g_Config->bCascadeShadow);
    shaderLightInject->setInt("cascadeLevel", g_Config->cascadeLevel);

    auto bounds = scene->GetSceneBoundingBox();
    auto center = bounds.center();
    float maxCoord = glm::compMax(glm::abs(center - bounds.pmax)) * 1.1;
    projMat = glm::ortho(-maxCoord, maxCoord, -maxCoord, maxCoord, 0.001f, 2 * maxCoord + 0.001f)
                     * glm::lookAt(center - glm::vec3(0, 0, maxCoord), center, glm::vec3(0, 1, 0));
    shaderLightInject->setMat4("invProject", glm::inverse(projMat));

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
                shaderLightInject->setMat4("lightMat", shadowMapCache.lightMats[0]);
            }
            else
            {
                glActiveTexture(GL_TEXTURE10);
                glBindTexture(GL_TEXTURE_2D_ARRAY, shadowMapCache.shadowPassDepthTexIds);
                for(Int cascadeIndex = 0; cascadeIndex < g_Config->cascadeLevel; cascadeIndex++)
                {
                    shaderLightInject->setMat4("cascadeMat[" + std::to_string(cascadeIndex) + "]", shadowMapCache.lightMats[cascadeIndex]);
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
    checkGLError();

    glBindImageTexture(0, radiance3DTexId, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
    checkGLError();
    glDispatchCompute(g_Config->VoxelSize / 8, g_Config->VoxelSize / 8, g_Config->VoxelSize / 8);
}

void VXGIPass::AddGenerateMipmapPass(FrameGraph &fg, FrameGraphBlackboard &blackboard, Scene *scene)
{
    return;
    Shader* shaderVoxelMipmapGenerate = g_ShaderManager.GetShader("VoxelMipmapGenerate");
    if(!shaderVoxelMipmapGenerate)
    {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, voxelSceneFBO);
    shaderVoxelMipmapGenerate->use();

    for(Int mipLevel = 1; mipLevel < g_Config->VoxelMipmapLvel; mipLevel++)
    {
        glBindImageTexture(0, radiance3DTexId, mipLevel - 1, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA8);
        glBindImageTexture(1, radiance3DTexId, mipLevel, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
        int div = pow(2, mipLevel);
        int workGroupSize = g_Config->VoxelSize / 8 / div;
        glDispatchCompute(workGroupSize, workGroupSize, workGroupSize);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
