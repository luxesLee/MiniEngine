#include "Renderer.h"
#include "RenderResource.h"
#include "ShaderManager.h"
#include "RenderDebug.h"

#include "Core/Scene.h"
#include "Core/Camera.h"
#include "Core/Config.h"

#include <fg/FrameGraph.hpp>
#include <fg/Blackboard.hpp>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#include "stb_image.h"
#include <random>

Renderer::Renderer(entt::registry& _reg) : reg(_reg)
{
    g_ShaderManager.InitShader();

    if(g_Config->curDenoise != DenoiseType::NONE)
    {
        denoisePass = CreateDenoisePass();
    }

    glViewport(0, 0, g_Config->screenWidth, g_Config->screenHeight);
    glGenBuffers(1, &CommonUBO);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, CommonUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, CommonUBO);
    glBufferData(GL_UNIFORM_BUFFER, 6 * sizeof(glm::mat4) + 2 * sizeof(glm::vec4) + sizeof(Int), NULL, GL_DYNAMIC_DRAW);
    glGenBuffers(1, &PathTracingUBO);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, PathTracingUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, PathTracingUBO);
    glBufferData(GL_UNIFORM_BUFFER, 3 * sizeof(int), NULL, GL_DYNAMIC_DRAW);
}

void Renderer::Update(Scene* scene)
{
    // we will update ubo and complete the culling in this function
    UpdateUBO(scene);
    DoCulling();

    if(denoisePass && g_Config->curDenoise != denoisePass->GetType())
    {
        delete denoisePass;
        denoisePass = CreateDenoisePass();
    }
    else if(!denoisePass && g_Config->curDenoise != DenoiseType::NONE)
    {
        denoisePass = CreateDenoisePass();
    }
}

void Renderer::Render(Scene *scene)
{
    if(g_Config->bRenderdocCapture)
    {
        RenderDebugger::startFrameCapture();
    }

    // if(g_Config->accumulateFrames == 0)
    // {
    //     glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    //     glClear(GL_COLOR_BUFFER_BIT);
    // }

    const auto defaultData = renderResource.get<DefaultData>();
    scene->curOutputTex = 1 - scene->curOutputTex;
    glBindFramebuffer(GL_FRAMEBUFFER, defaultData.defaultFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, defaultData.defaultColorData[scene->curOutputTex], 0);

    FrameGraph fg;
    FrameGraphBlackboard blackboard;

    // todo: forward and deferred
    switch (g_Config->lightMode)
    {
    case LightMode::Forward: ForwardRendering(fg, blackboard, scene); break;
    case LightMode::Deferred:
    case LightMode::TiledDeferred:
    case LightMode::ClusterDeferred:
        DeferredRendering(fg, blackboard, scene);
        break;
    case LightMode::PathTracing: PathTracing(fg, blackboard, scene); break;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, defaultData.defaultFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, defaultData.defaultColorData[1 - scene->curOutputTex], 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, defaultData.defaultFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, g_Config->wholeWidth, g_Config->screenHeight, 0, 0, g_Config->wholeWidth, g_Config->screenHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    if(g_Config->bRenderdocCapture)
    {
        g_Config->bRenderdocCapture = false;
        RenderDebugger::endFrameCapture();
    }
}

void Renderer::Resize()
{
    glViewport(0, 0, g_Config->screenWidth, g_Config->screenHeight);
}

void Renderer::UpdateUBO(Scene* scene)
{
    if(CommonUBO)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, CommonUBO);
        // Model
        glm::mat4 indentity = glm::mat4(1.0f);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &indentity);
        // View
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &g_Camera->GetViewMatrix());
        // Projection
        glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), sizeof(glm::mat4), &g_Camera->GetProjectionMatrix());
        // ProjectionView
        glBufferSubData(GL_UNIFORM_BUFFER, 3 * sizeof(glm::mat4), sizeof(glm::mat4), &(g_Camera->GetProjectionMatrix() * g_Camera->GetViewMatrix()));
        // invProjection
        glBufferSubData(GL_UNIFORM_BUFFER, 4 * sizeof(glm::mat4), sizeof(glm::mat4), &glm::inverse(g_Camera->GetProjectionMatrix()));
        // invViewProjection
        glBufferSubData(GL_UNIFORM_BUFFER, 5 * sizeof(glm::mat4), sizeof(glm::mat4), &glm::inverse(g_Camera->GetProjectionMatrix() * g_Camera->GetViewMatrix()));
        // screenAndInvScreen
        glBufferSubData(GL_UNIFORM_BUFFER, 6 * sizeof(glm::mat4), sizeof(glm::vec4), &g_Camera->GetScreenAndInvScreen());
        // cameraPosition
        glBufferSubData(GL_UNIFORM_BUFFER, 6 * sizeof(glm::mat4) + sizeof(glm::vec4), sizeof(glm::vec4), &g_Camera->Position);
        // lightNum
        int lightNum = scene->getLightNum();
        glBufferSubData(GL_UNIFORM_BUFFER, 6 * sizeof(glm::mat4) + 2 * sizeof(glm::vec4), sizeof(Int), &lightNum);
    }

    if(PathTracingUBO)
    {
        g_Config->accumulateFrames++;
        glBindBuffer(GL_UNIFORM_BUFFER, PathTracingUBO);
        int maxDepth = 1;
        int topBVHIndex = scene->getTopBVHIndex();
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Int), &g_Config->maxRayTracingDepth);
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(Int), sizeof(Int), &g_Config->accumulateFrames);
        glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(Int), sizeof(Int), &topBVHIndex);
    }
}

void Renderer::DoCulling()
{
}

void Renderer::ForwardRendering(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene)
{
    
}

void Renderer::DeferredRendering(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene)
{
    // BasePass
    basePass.AddPass(fg, blackboard, scene, renderResource);

    if(g_Config->bSSAO)
    {
        aoRenderer.AddPass(fg, blackboard, scene, renderResource);
    }

    // ShadowMap
    if(g_Config->bShadeShadow || g_Config->bVXGI)
    {
        glViewport(0, 0, g_Config->shadowDepthWidth, g_Config->shadowDepthHeight);
        shadowRenderer.AddPass(fg, blackboard, scene, renderResource);
        glViewport(0, 0, g_Config->screenWidth, g_Config->screenHeight);
    }

    // VXGI
    if(g_Config->bVXGI)
    {
        glViewport(0, 0, g_Config->VoxelSize, g_Config->VoxelSize);
        vxgiPass.AddBuildPass(fg, blackboard, scene, renderResource);
        glViewport(0, 0, g_Config->screenWidth, g_Config->screenHeight);
    }

    // Direct Lighting
    switch (g_Config->lightMode)
    {
    case LightMode::Deferred: deferredLightingPass.AddPass(fg, blackboard, scene, renderResource); break;
    case LightMode::TiledDeferred: break;
    case LightMode::ClusterDeferred: break;
    }

    if(g_Config->bVXGI)
    {
        vxgiPass.AddIndirectLightingPass(fg, blackboard, scene);
    }

    if(g_Config->bSSR)
    {

    }

    // Post Process
    postProcessPass.AddPass(fg, blackboard, scene);

    // Debug
    if(g_Config->debugMode != DebugMode::NONE)
    {
        DebugRendering(fg, blackboard, scene);
    }
}

void Renderer::DebugRendering(FrameGraph &fg, FrameGraphBlackboard &blackboard, Scene *scene)
{
    const auto defaultData = renderResource.get<DefaultData>();
    glBindFramebuffer(GL_FRAMEBUFFER, defaultData.defaultFBO);

    switch (g_Config->debugMode)
    {
    case DebugMode::DebugShadow:
        shadowRenderer.AddPassVisualizeShadowMap(scene);
        break;
    case DebugMode::DebugVXGI:
        glBindVertexArray(defaultData.defaultVAO);
        vxgiPass.AddDebugPass(fg, blackboard, scene);
    default:
        break;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::PathTracing(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene)
{
    glViewport(0, 0, g_Config->screenWidth, g_Config->screenHeight);
    pathTracingPass.AddPass(fg, blackboard, scene, renderResource);
    if(g_Config->curToneMapping != ToneMappingType::NONE)
    {
        toneMappingPass.AddPass(fg, blackboard, scene, renderResource);
    }

    if(denoisePass)
    {
        denoisePass->AddPass(scene, renderResource);
    }
}

void Renderer::InitRenderResource()
{
    InitDefaultResourceData();

    if(g_Config->lightMode == LightMode::PathTracing)
    {
        InitPathTracingRenderingResourceData();
    }
    else if(g_Config->lightMode == LightMode::Forward)
    {

    }
    else
    {
        InitDeferredRenderingResourceData();
    }

    if(g_Config->bShadeShadow)
    {
        InitShadowMapRenderingResourceData();
    }

    if(g_Config->bSSAO)
    {
        InitSSAORenderingResourceData();
    }

    if(g_Config->bVXGI)
    {
        InitVXGIRenderingResourceData();
    }
}

void Renderer::DeleteRenderResource()
{
    
}

void Renderer::InitDefaultResourceData()
{
    GLuint defaultFBO = generateFBO();

    TextureDesc screenTexDesc{g_Config->wholeWidth, g_Config->screenHeight, 0, TextureType::TEXTURE_2D, TextureFormat::RGBA32F,
                            LINEAR_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 0, nullptr, DataFormat::DataFormat_RGBA, DataType::FLOAT};
    GPUTexture outputTex0 = generateTexture(screenTexDesc);
    GPUTexture outputTex1 = generateTexture(screenTexDesc);
    
    TextureDesc depthTexDesc{g_Config->wholeWidth, g_Config->screenHeight, 0, TextureType::TEXTURE_2D, TextureFormat::DEPTH24_STENCIL8,
                            NEAREST_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 0, nullptr, DataFormat::DataFormat_DEPTH_STENCIL, DataType::UNSIGNED_INT_24_8};
    GPUTexture outputDepthTex = generateTexture(depthTexDesc);

    GLuint defaultVAO = generateVAO();

    renderResource.add<DefaultData>(defaultFBO, outputTex0.texId, outputTex1.texId, outputDepthTex.texId, defaultVAO);
}

void Renderer::InitDeferredRenderingResourceData()
{
    GLuint gBufferFBO = generateFBO();

    TextureDesc gBufferTexDesc{g_Config->screenWidth, g_Config->screenHeight, 0, TextureType::TEXTURE_2D, TextureFormat::RGBA32F,
                        NEAREST_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 0, nullptr, DataFormat::DataFormat_RGBA, DataType::FLOAT};
    GPUTexture gBufferTex0 = generateTexture(gBufferTexDesc);
    GPUTexture gBufferTex1 = generateTexture(gBufferTexDesc);
    GPUTexture gBufferTex2 = generateTexture(gBufferTexDesc);
    GPUTexture gBufferTex3 = generateTexture(gBufferTexDesc);
    GPUTexture gBufferTex4 = generateTexture(gBufferTexDesc);

    TextureDesc depthTexDesc{g_Config->screenWidth, g_Config->screenHeight, 0, TextureType::TEXTURE_2D, TextureFormat::DEPTH24_STENCIL8,
                        NEAREST_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 0, nullptr, DataFormat::DataFormat_DEPTH_STENCIL, DataType::UNSIGNED_INT_24_8};
    GPUTexture gBufferDepthTex = generateTexture(depthTexDesc);

    glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gBufferTex0.texId, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gBufferTex1.texId, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gBufferTex2.texId, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gBufferTex3.texId, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, gBufferTex4.texId, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, gBufferDepthTex.texId, 0);

    GLenum DrawBuffers[5] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4};
    glDrawBuffers(5, DrawBuffers);

    renderResource.add<GBufferData>(gBufferFBO, gBufferTex0.texId, gBufferTex1.texId, gBufferTex2.texId, 
                                    gBufferTex3.texId, gBufferTex4.texId, gBufferDepthTex.texId);
}

void Renderer::InitPathTracingRenderingResourceData()
{
    GLuint pathTracingFBO = generateFBO();

    TextureDesc screenTexDesc{g_Config->wholeWidth, g_Config->screenHeight, 0, TextureType::TEXTURE_2D, TextureFormat::RGBA32F,
                            LINEAR_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 0, nullptr, DataFormat::DataFormat_RGBA, DataType::FLOAT};
    GPUTexture pathTracingTex = generateTexture(screenTexDesc);
    GPUTexture accumTex = generateTexture(screenTexDesc);

    glBindFramebuffer(GL_FRAMEBUFFER, pathTracingFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pathTracingTex.texId, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, accumTex.texId, 0);
    GLenum DrawBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, DrawBuffers);

    renderResource.add<PathTracingData>(pathTracingFBO, pathTracingTex.texId, accumTex.texId);
}

void Renderer::InitShadowMapRenderingResourceData()
{
    GLuint shadowPassFBO = generateFBO();
    renderResource.add<ShadowMapData>(shadowPassFBO);
}

void Renderer::InitSSAORenderingResourceData()
{
    GLuint ssaoFBO = generateFBO();
    GLuint ssaoBlurFBO = generateFBO();

    std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++)
    {
        glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f);
        ssaoNoise.push_back(noise);
    }
    TextureDesc ssaoNoisyTexDesc{4, 4, 0, TextureType::TEXTURE_2D, TextureFormat::RGBA32F, 
                                NEAREST_REPEAT_SAMPLER, 0, ssaoNoise.data(), DataFormat::DataFormat_RGB, DataType::FLOAT};
    GPUTexture ssaoNoisyTex = generateTexture(ssaoNoisyTexDesc);
            
    TextureDesc ssaoInterMediateTexDesc{g_Config->screenWidth, g_Config->screenHeight, 0, TextureType::TEXTURE_2D, TextureFormat::RED,
                                    NEAREST_REPEAT_SAMPLER, 0, nullptr, DataFormat::DataFormat_RED, DataType::FLOAT};
    GPUTexture ssaoInterMediateTex = generateTexture(ssaoInterMediateTexDesc);

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoInterMediateTex.texId, 0);

    renderResource.add<SSAOData>(ssaoFBO, ssaoBlurFBO, ssaoNoisyTex.texId, ssaoInterMediateTex.texId);
}

void Renderer::InitVXGIRenderingResourceData()
{
    GLuint vxgiFBO = generateFBO();

    TextureDesc desc1{g_Config->VoxelSize, g_Config->VoxelSize, g_Config->VoxelSize, TextureType::TEXTURE_3D, TextureFormat::RGBA8,
                    LINEAR_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 1};
    GPUTexture albedo3DTex = generateTexture(desc1);
    GPUTexture normal3DTex = generateTexture(desc1);

    TextureDesc desc2{g_Config->VoxelSize, g_Config->VoxelSize, g_Config->VoxelSize, TextureType::TEXTURE_3D, TextureFormat::RGBA8,
            LINEAR_MIPMAP_LINEAR_LINEAR_CLAMP_TO_BORDER_BLACK_BORDER_SAMPLER, Int(std::log2(g_Config->VoxelSize / 8))};
    GPUTexture radiance3DTex = generateTexture(desc2);

    renderResource.add<VXGIData>(vxgiFBO, albedo3DTex.texId, normal3DTex.texId, radiance3DTex.texId);
}

void Renderer::UpdateSceneBuffers()
{
    if(true)
    {
        std::vector<Material> materials_GPU;
        const auto materialsView = reg.view<Material>();
        for(auto matView : materialsView)
        {
            Material& mat = reg.get<Material>(matView);
            materials_GPU.emplace_back(mat);
        }
        std::reverse(materials_GPU.begin(), materials_GPU.end());
        TextureDesc matDesc{materials_GPU.size() * sizeof(Material) / sizeof(vec4), 1, 0, TextureType::TEXTURE_2D, TextureFormat::RGBA32F, 
            NEAREST_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 0, materials_GPU.data(), DataFormat::DataFormat_RGBA, DataType::FLOAT};
        GPUTexture matTex = generateTexture(matDesc);
        renderResource.add<GPUMaterialData>(matTex.texId);
    }

    {
        std::vector<glm::mat4> transforms_GPU;
        const auto instancesView = reg.view<MeshInstance>();
        for(auto instanceView : instancesView)
        {
            MeshInstance& meshInstance = reg.get<MeshInstance>(instanceView);
            transforms_GPU.emplace_back(meshInstance.transform);
        }
        std::reverse(transforms_GPU.begin(), transforms_GPU.end());
        TextureDesc transformDesc{transforms_GPU.size() * sizeof(mat4) / sizeof(vec4), 1, 0, TextureType::TEXTURE_2D, TextureFormat::RGBA32F, 
            NEAREST_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 0, transforms_GPU.data(), DataFormat::DataFormat_RGBA, DataType::FLOAT};
        GPUTexture transformTex = generateTexture(transformDesc);
        renderResource.add<GPUTransformData>(transformTex.texId);
    }

    {
        std::vector<Light> lights_GPU;
        const auto lightsView = reg.view<Light>();
        for(auto lightView : lightsView)
        {
            Light& light = reg.get<Light>(lightView);
            lights_GPU.emplace_back(light);
        }
        std::reverse(lights_GPU.begin(), lights_GPU.end());
        TextureDesc lightDesc{lights_GPU.size() * sizeof(Light), 0, 0, TextureType::Buffer, TextureFormat::RGBA32F,
            NEAREST_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 0, lights_GPU.data()};
        GPUTexture lightTex = generateTexture(lightDesc);
        renderResource.add<GPULightData>(lightTex.texId);
    }

    if(g_Config->bTexCombine || g_Config->lightMode == LightMode::PathTracing)
    {
        const auto texturesView = reg.view<Texture*>();
        std::vector<Uchar> textureMapsArray;
        Int reqWidth = g_Config->texWidth, reqHeight = g_Config->texHeight;
        Int texBytes = reqWidth * reqHeight * 4;
        textureMapsArray.resize(texBytes * texturesView.size());

        Int count = 1, numTextures = texturesView.size();
        for(auto textureView : texturesView)
        {
            Texture* texture = reg.get<Texture*>(textureView);
            
            if(texture->width != reqWidth || texture->height != reqHeight)
            {
                Uchar* resizedTex = new Uchar[texBytes];
                stbir_resize_uint8(&texture->data[0], texture->width, texture->height, 0, resizedTex, reqWidth, reqHeight, 0, 4);
                std::copy(resizedTex, resizedTex + texBytes, &textureMapsArray[(numTextures - count) * texBytes]);
                delete[] resizedTex;
            }
            else
            {
                std::copy(texture->data.begin(), texture->data.end(), &textureMapsArray[(numTextures - count) * texBytes]);
            }

            count++;
        }
    
        TextureDesc textureArrayDesc{g_Config->texWidth, g_Config->texHeight, texturesView.size(), TextureType::TEXTURE_2D_ARRAY, TextureFormat::RGBA8,
            LINEAR_REPEAT_SAMPLER, 0, textureMapsArray.data(), DataFormat::DataFormat_RGBA, DataType::UNSIGNED_BYTE};
        GPUTexture textureArrayTex = generateTexture(textureArrayDesc);
        renderResource.add<GPUCombinedTextureData>(textureArrayTex.texId);
    }

    InitMeshBuffer();
}

void Renderer::InitMeshBuffer()
{

    if(g_Config->lightMode != LightMode::PathTracing)
    {
        // std::sort(meshInstances.begin(), meshInstances.end(), [&](MeshInstance& instance1, MeshInstance& instance2)
        // {
        //     return instance1.meshID != instance2.meshID ? instance1.meshID < instance2.meshID : instance1.materialID < instance2.materialID;
        // });
    
        // auto CopyData = [&](MeshBatch* meshBatch, auto& meshes, auto& meshInstances, int index)
        // {
        //     meshBatch->instanceCount++;
        //     meshBatch->vertices.insert(meshBatch->vertices.end(), 
        //         meshes[meshInstances[index].meshID]->vertices.begin(), 
        //         meshes[meshInstances[index].meshID]->vertices.end());
        //     meshBatch->indices.insert(meshBatch->indices.end(), 
        //         meshes[meshInstances[index].meshID]->indices.begin(), 
        //         meshes[meshInstances[index].meshID]->indices.end());
        //     meshBatch->normals.insert(meshBatch->normals.end(), 
        //         meshes[meshInstances[index].meshID]->normals.begin(), 
        //         meshes[meshInstances[index].meshID]->normals.end());
        //     meshBatch->uvs.insert(meshBatch->uvs.end(), 
        //         meshes[meshInstances[index].meshID]->uvs.begin(), 
        //         meshes[meshInstances[index].meshID]->uvs.end());
        //     for(int i = 0; i < meshes[meshInstances[index].meshID]->vertices.size(); i++)
        //     {
        //         meshBatch->materialIDs.push_back(meshInstances[index].materialID);
        //     }
        // };
    
        // std::vector<MeshBatch*> meshBatches;
        // MeshBatch* meshBatch = new MeshBatch;
        // Int pre = 0;
        // CopyData(meshBatch, meshes, meshInstances, pre);
        // for(Int i = 1; i < meshInstances.size(); i++)
        // {
        //     if(meshInstances[i].meshID != meshInstances[pre].meshID)
        //     {
        //         meshBatches.push_back(meshBatch);
        //         meshBatch = new MeshBatch;
        //     }
        //    CopyData(meshBatch, meshes, meshInstances, i);
        // }
        // meshBatches.push_back(meshBatch);
    }
    else
    {
        InitPathTracingMeshBuffer();
    }
}

void Renderer::InitPathTracingMeshBuffer()
{
    // TextureDesc verticeTexDesc{vertices.size() * sizeof(vec3), 0, 0, TextureType::Buffer, TextureFormat::RGB32F,
    //             NEAREST_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 0, vertices.data()};
    // GPUTexture verticeTex = generateTexture(verticeTexDesc);

    // TextureDesc indiceTexDesc{indices.size() * sizeof(Indice), 0, 0, TextureType::Buffer, TextureFormat::RGB32I,
    //             NEAREST_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 0, indices.data()};
    // GPUTexture indicesTex = generateTexture(indiceTexDesc);

    // TextureDesc normalTexDesc{normals.size() * sizeof(vec3), 0, 0, TextureType::Buffer, TextureFormat::RGB32F,
    //             NEAREST_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 0, normals.data()};
    // GPUTexture normalTex = generateTexture(normalTexDesc);

    // TextureDesc uvTexDesc{uvs.size() * sizeof(vec2), 0, 0, TextureType::Buffer, TextureFormat::RG32F,
    //             NEAREST_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 0, uvs.data()};
    // GPUTexture uvsTex = generateTexture(uvTexDesc);

    // TextureDesc bvhTexDesc{bvhTranslator.nodes.size() * sizeof(BvhTranslator::Node), 0, 0, TextureType::Buffer, TextureFormat::RGB32F,
    //             NEAREST_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 0, bvhTranslator.nodes.data()};
    // GPUTexture bvhTex = generateTexture(bvhTexDesc);

    // PathTracingData& pathTracingData = renderResource.get<PathTracingData>();
    // pathTracingData.vertexData = verticeTex.texId;
    // pathTracingData.indiceData = indicesTex.texId;
    // pathTracingData.normalData = normalTex.texId;
    // pathTracingData.bvhData = bvhTex.texId;
}

void Renderer::InitGPUBuffer()
{

}
