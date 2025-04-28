#include "PipelineStateObject.h"

using PSOBuilder = PipelineStateObject::PSOBuilder;

PSOBuilder &PipelineStateObject::PSOBuilder::setShaderProgram(GLuint program)
{
    this->program = program;
    return *this;
}

PSOBuilder &PipelineStateObject::PSOBuilder::setDepthStencil(const DepthStencilState &depthStencilState)
{
    this->depthStencilState = depthStencilState;
    return *this;
}

PSOBuilder &PipelineStateObject::PSOBuilder::setRasterState(const RasterizerState &rasterState)
{
    this->rasterState = rasterState;
    return *this;
}

PSOBuilder &PipelineStateObject::PSOBuilder::setBlendState(Uint attachNum, const BlendState &blendState)
{
    if(attachNum < kMaxNumBlendStates)
    {
        blendStates[attachNum] = blendState;
    }
    return *this;
}

PipelineStateObject PipelineStateObject::PSOBuilder::build()
{
    PipelineStateObject pso;
    pso.program = program;
    pso.depthStencilState = depthStencilState;
    pso.rasterState = rasterState;
    pso.blendStates = blendStates;

    return pso;
}

void PipelineStateObject::setState()
{
    
}
