#include "ShadowRenderer.h"
#include "Render/ShaderManager.h"
#include "Core/Shader.h"
#include "Core/Camera.h"
#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"

void ShadowRenderer::InitLightMatrix(Scene *scene)
{
}

void ShadowRenderer::AddPass(FrameGraph &fg, FrameGraphBlackboard &blackboard, Scene *scene)
{
    Shader* shaderShadowDepth = g_ShaderManager.GetShader("ShadowDepth");
    if(!shaderShadowDepth)
    {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, scene->shadowPassFBO);

    glClearDepth(1.0);
    glDepthFunc(GL_LEQUAL);
    glCullFace(GL_FRONT);  // peter panning

    auto lights = scene->GetSceneLights();
    auto& lightMats = scene->GetLightMats();
    lightMats.resize(lights.size());

    shaderShadowDepth->use();

    for(int i = 0; i < lights.size(); i++)
    {
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, scene->shadowPassDepthTexIds[i], 0);
        glClear(GL_DEPTH_BUFFER_BIT);

        lightMats[i] = GetLightMatrix(&lights[i], scene);
        shaderShadowDepth->setMat4("lightTransform", lightMats[i]);

        if(lights[i].type == LightType::DIRECTIONAL_LIGHT)
        {
            if(g_Config->bCascadeShadow)
            {

            }
            else
            {

            }
        }
        else if(lights[i].type == LightType::POINT_LIGHT)
        {

        }


        auto meshBatches = scene->GetMeshBatches();
        Int instanceBase = 0, curInstanceCount = 0;
        for(auto& meshBatch : meshBatches)
        {
            curInstanceCount = meshBatch->GetInstanceCount();
            shaderShadowDepth->setInt("instanceBase", instanceBase);
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
    }
}

std::vector<glm::vec4> GetFrustumPoints(const glm::mat4& projViewMat)
{
    const auto inv = glm::inverse(projViewMat);

    std::vector<glm::vec4> frustumCorners;
    for (unsigned int x = 0; x < 2; ++x)
    {
        for (unsigned int y = 0; y < 2; ++y)
        {
            for (unsigned int z = 0; z < 2; ++z)
            {
                // NDC->WCS
                const glm::vec4 pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                frustumCorners.push_back(pt / pt.w);
            }
        }
    }

    return frustumCorners;
}

void ShadowRenderer::AddPassVisualizeShadowMap(const std::vector<glm::mat4>& lightMatrices)
{
    std::vector<GLuint> visualizerVAOs;
    std::vector<GLuint> visualizerVBOs;
    std::vector<GLuint> visualizerEBOs;

    const GLuint indices[] = {
        0, 2, 3,
        0, 3, 1,
        4, 6, 2,
        4, 2, 0,
        5, 7, 6,
        5, 6, 4,
        1, 3, 7,
        1, 7, 5,
        6, 7, 3,
        6, 3, 2,
        1, 5, 4,
        0, 1, 4
    };

    const glm::vec4 colors[] = {
        {1.0, 0.0, 0.0, 0.5f},
        {0.0, 1.0, 0.0, 0.5f},
        {0.0, 0.0, 1.0, 0.5f},
    };

    visualizerVAOs.resize(8);
    visualizerEBOs.resize(8);
    visualizerVBOs.resize(8);

    for (int i = 0; i < lightMatrices.size(); ++i)
    {
        const auto corners = GetFrustumPoints(lightMatrices[i]);
        std::vector<glm::vec3> vec3s;
        for (const auto& v : corners)
        {
            vec3s.push_back(glm::vec3(v));
        }

        glGenVertexArrays(1, &visualizerVAOs[i]);
        glGenBuffers(1, &visualizerVBOs[i]);
        glGenBuffers(1, &visualizerEBOs[i]);

        glBindVertexArray(visualizerVAOs[i]);

        glBindBuffer(GL_ARRAY_BUFFER, visualizerVBOs[i]);
        glBufferData(GL_ARRAY_BUFFER, vec3s.size() * sizeof(glm::vec3), &vec3s[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, visualizerEBOs[i]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 36 * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

        glBindVertexArray(visualizerVAOs[i]);

        Shader* shader = g_ShaderManager.GetShader("VisualizeShadowMap");
        shader->use();
        shader->setVec4("color", colors[i % 3]);

        glDrawElements(GL_TRIANGLES, GLsizei(36), GL_UNSIGNED_INT, 0);

        glDeleteBuffers(1, &visualizerVBOs[i]);
        glDeleteBuffers(1, &visualizerEBOs[i]);
        glDeleteVertexArrays(1, &visualizerVAOs[i]);

        glBindVertexArray(0);
    }

    visualizerVAOs.clear();
    visualizerEBOs.clear();
    visualizerVBOs.clear();
}

const glm::mat4 &ShadowRenderer::GetLightMatrix(Light *light, Scene* scene)
{
    if(light->type == LightType::DIRECTIONAL_LIGHT)
    {
        return GetDirectionalLightMatrix(light, scene);
    }
    else if(light->type == LightType::POINT_LIGHT)
    {
        // return GetPointLightMatrix(light);
    }
    else if(light->type == LightType::SPOT_LIGHT)
    {
        return GetSpotLightMatrix(light);
    }
}

const glm::mat4 &ShadowRenderer::GetDirectionalLightMatrix(Light *light, Scene* scene)
{
    // 这里直接使用已加载的场景包围盒
    // 可以使整个场景都出现在阴影图上
    RadeonRays::bbox sceneBox = scene->GetSceneBoundingBox();
    glm::vec3 center = sceneBox.center();

    glm::mat4 V = glm::lookAt(center, center + vec3(light->direction), glm::vec3(0, 1, 0));

    auto extent = RadeonRays::transformBy(V, sceneBox);
    glm::mat4 P = glm::ortho(extent.pmin.x, extent.pmax.x, extent.pmin.y, extent.pmax.y, 
                    extent.pmin.z, 1.1f * extent.pmax.z);

    return P * V;
}

const std::vector<glm::mat4>& ShadowRenderer::GetPointLightMatrix(Light *light)
{
    std::vector<glm::mat4> pointsMat(6);
    glm::vec3 position = light->position, target, up;
    glm::mat4 P = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, light->range), V;
    for(Int i = 0; i < 6; i++)
    {
        switch (i)
        {
        case 0:
            target = position + glm::vec3(1, 0, 0);
            up = glm::vec3(0, 1, 0);
            break;
        case 1:
            target = position + glm::vec3(-1, 0, 0);
            up = glm::vec3(0, 1, 0);
            break;
        case 2:
            target = position + glm::vec3(0, 1, 0);
            up = glm::vec3(0, 0, -1);
            break;
        case 3:
            target = position + glm::vec3(0, -1, 0);
            up = glm::vec3(0, 0, 1);
            break;
        case 4:
            target = position + glm::vec3(0, 0, 1);
            up = glm::vec3(0, 1, 0);
            break;
        case 5:
            target = position + glm::vec3(0, 0, -1);
            up = glm::vec3(0, 1, 0);
            break;
        default:
            break;
        }
        V = glm::lookAt(position, target, up);
        pointsMat[i] = P * V;
    }

    return pointsMat;
}

const glm::mat4 &ShadowRenderer::GetSpotLightMatrix(Light *light)
{
    return glm::mat4(1.0f);
}

const glm::mat4 &ShadowRenderer::GetQuadLightMatrix(Light *light)
{
    return glm::mat4(1.0f);
}

const std::vector<glm::mat4> &ShadowRenderer::GetCascadeLightMatrix(Light *light)
{
    std::vector<glm::mat4> ret(g_Config->cascadeLevel);
    std::vector<std::pair<Float, Float>> cascadePlane = {
        {g_Config->cameraZNear, g_Config->cameraZFar / 5},
        {g_Config->cameraZFar / 5, g_Config->cameraZFar / 3},
        {g_Config->cameraZFar / 3, g_Config->cameraZFar / 2},
        {g_Config->cameraZFar / 2, g_Config->cameraZFar}};

    auto GetCascadeMat = [=](Light *light, Float ZNear, Float ZFar)
    {
        float near = ZNear;
        float far = ZFar;
        if(near > far)
        {
            std::swap(near, far);
        }
        glm::mat4 proj = glm::perspective(glm::radians(g_Camera->Zoom), 
                                    (g_Camera->screenWidth * 1.0f / g_Camera->screenHeight), 
                                    near, far);

        RadeonRays::bbox bound;
        glm::mat4 viewMat = g_Camera->GetViewMatrix();
        const auto ptsWs = GetFrustumPoints(proj * g_Camera->GetViewMatrix());
        for(auto& pt : ptsWs)
        {
            bound.grow(pt);
        }

        glm::vec3 center = bound.center();

        glm::mat4 V = glm::lookAt(center, center + vec3(light->direction), glm::vec3(0, 1, 0));

        RadeonRays::bbox extent;
        for(int i = 0; i < ptsWs.size(); i++)
        {
            auto pt = V * ptsWs[i];
            extent.grow(pt);
        }

        // 需要拉长远近平面，使相邻阴影贴图存在重合
        glm::mat4 P = glm::ortho(extent.pmin.x, extent.pmax.x, extent.pmin.y, extent.pmax.y, 
                        extent.pmin.z, 1.5f * extent.pmax.z);
        
        return P * V;
    };

    for(int i = 0; i < ret.size(); i++)
    {
        ret[i] = GetCascadeMat(light, cascadePlane[i].first, cascadePlane[i].second);
    }
    return ret;
}
