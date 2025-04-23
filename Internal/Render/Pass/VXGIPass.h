#pragma once
#include "glad/glad.h"
#include "mat4x4.hpp"

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

    void AddBuildPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);

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

    glm::mat4 projMat;
};
