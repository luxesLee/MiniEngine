#pragma once
#include "mat4x4.hpp"
#include "Core/Scene.h"

class FrameGraph;
class FrameGraphBlackboard;
class Scene;

class ShadowRenderer
{
public:
    ShadowRenderer() {}

    ~ShadowRenderer() {}

    void InitLightMatrix(Scene* scene);

    void AddPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene, RenderResource& renderResource);

    void AddPassVisualizeShadowMap(Scene* scene);

private:
    glm::mat4& GetDirectionalLightMatrix(Light* light, Scene* scene);

    std::vector<glm::mat4> GetPointLightMatrix(Light* light);
    
    glm::mat4& GetSpotLightMatrix(Light* light);
    
    glm::mat4& GetQuadLightMatrix(Light* light);
    
    std::vector<glm::mat4> GetCascadeLightMatrix(Light* light);
};