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
    Shader* shaderToneMapping = g_ShaderManager.GetShader("ToneMapping");
    if(!shaderToneMapping)
    {
        return;
    }






}

void ToneMappingPass::Resize(uint32_t width, uint32_t height)
{
    
}
