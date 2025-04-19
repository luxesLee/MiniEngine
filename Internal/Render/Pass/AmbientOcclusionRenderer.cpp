#include "AmbientOcclusionRenderer.h"

void AmbientOcclusionRenderer::AddPass(FrameGraph &fg, FrameGraphBlackboard &blackboard, Scene *scene)
{
    ssaoPass.AddPass(fg, blackboard, scene);
}
