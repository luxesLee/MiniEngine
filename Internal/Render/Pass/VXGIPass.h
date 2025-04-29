#pragma once
#include "glad/glad.h"
#include "mat4x4.hpp"
#include "Core/Scene.h"

class FrameGraph;
class FrameGraphBlackboard;
class Scene;

class VXGIPass
{
public:
    VXGIPass() { Init();}

    ~VXGIPass() {}

    void Init();

    void Delete();

    void AddBuildPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene, RenderResource& renderResource);

    void AddIndirectLightingPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);

    void AddDebugPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);

private:
    void AddVoxelSceneBuildPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);

    void AddLightInjectPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);
    
    void AddGenerateMipmapPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);

private:
    GLuint voxelSceneFBO;

    GLuint albedo3DTexId;
    GLuint normal3DTexId;
    GLuint radiance3DTexId;

    GLuint curOutputTexId;

    GLuint screenVAO;

    GLuint gBufferTexId[4];

    glm::mat4 projMat;
};
