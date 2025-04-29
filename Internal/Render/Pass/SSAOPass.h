#pragma once
#include "glad/glad.h"
#include "vec3.hpp"
#include <vector>
#include "Core/Scene.h"

class FrameGraph;
class FrameGraphBlackboard;

class SSAOPass
{
public:
    SSAOPass();
    
    ~SSAOPass();

    void AddPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene, RenderResource& renderResource);

private:
    void AddSSAOPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);
    void AddSSAOBlurPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);

private:
    GLuint ssaoFBO;
    GLuint ssaoBlurFBO;
    GLuint ssaoNoisyTexId;
    GLuint ssaoIntermediateTexId;
    std::vector<glm::vec3> ssaoKernel;
    GLuint vao;

    GLuint gBufferTexId[4];
};
