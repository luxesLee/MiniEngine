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

    void AddPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene);

    void AddPassVisualizeShadowMap(const std::vector<glm::mat4>& lightMatrices);

    const std::vector<glm::mat4>& GetLightMats() const {return lightMats;}

private:
    const glm::mat4& GetLightMatrix(Light* light, Scene* scene);
    const glm::mat4& GetDirectionalLightMatrix(Light* light, Scene* scene);
    const std::vector<glm::mat4>& GetPointLightMatrix(Light* light);
    const glm::mat4& GetSpotLightMatrix(Light* light);
    const glm::mat4& GetQuadLightMatrix(Light* light);
    const std::vector<glm::mat4>& GetCascadeLightMatrix(Light* light);

private:
    std::vector<glm::mat4> lightMats;
};