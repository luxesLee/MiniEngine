#include "VXGIPass.h"
#include "Render/ShaderManager.h"
#include "Core/Shader.h"
#include "Core/Camera.h"
#include "Core/Scene.h"

#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"

void VXGIPass::AddPass(FrameGraph &fg, FrameGraphBlackboard &blackboard, Scene *scene)
{
    AddVoxelSceneBuildPass(fg, blackboard, scene);
    AddLightInjectPass(fg, blackboard, scene);
    AddIndirectLightingPass(fg, blackboard, scene);
}

void VXGIPass::AddVoxelSceneBuildPass(FrameGraph &fg, FrameGraphBlackboard &blackboard, Scene *scene)
{
    
}

void VXGIPass::AddLightInjectPass(FrameGraph &fg, FrameGraphBlackboard &blackboard, Scene *scene)
{
}

void VXGIPass::AddIndirectLightingPass(FrameGraph& fg, FrameGraphBlackboard& blackboard, Scene* scene)
{

}
