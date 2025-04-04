#include "ToneMappingPass.h"
#include "Render/ShaderManager.h"
#include <fg/FrameGraph.hpp>

ToneMappingPass::ToneMappingPass()
{
}

ToneMappingPass::~ToneMappingPass()
{
}

void ToneMappingPass::AddPass(FrameGraph &fg)
{
    Shader* shader = g_ShaderManager.GetShader("ToneMapping");
    if(shader)
    {

    }
}

void ToneMappingPass::Resize(uint32_t width, uint32_t height)
{
    
}
