#include "AmbientOcclusionRenderer.h"

void AmbientOcclusionRenderer::AddPass(FrameGraph &fg, FrameGraphBlackboard &blackboard, Scene *scene, RenderResource& renderResource)
{
    ssaoPass.AddPass(fg, blackboard, scene, renderResource);
}
