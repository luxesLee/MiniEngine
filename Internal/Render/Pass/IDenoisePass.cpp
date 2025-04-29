#include "IDenoisePass.h"
#include "Render/RenderResource.h"
#include "Core/Config.h"
#include <iostream>
#include "Util/Timer.h"

SVGFDenoisePass::SVGFDenoisePass()
{

}

void SVGFDenoisePass::AddPass(Scene *scene, RenderResource& renderResource)
{

}

OIDNDenoisePass::OIDNDenoisePass()
{
    device = oidn::newDevice();
    device.commit();
    filter = device.newFilter("RT");

    inputFrameVec.resize(4 * g_Config->screenHeight * g_Config->screenWidth);
    outputFrameVec.resize(4 * g_Config->screenHeight * g_Config->screenWidth);
}

OIDNDenoisePass::~OIDNDenoisePass()
{

}

void OIDNDenoisePass::AddPass(Scene *scene, RenderResource& renderResource)
{
    // 回读前一帧输出
    const auto defaultData = renderResource.get<DefaultData>();
    GLuint curOutputTexId = defaultData.defaultColorData[1 - scene->curOutputTex];

    glBindTexture(GL_TEXTURE_2D, curOutputTexId);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, inputFrameVec.data());

    filter.setImage("color", inputFrameVec.data(), oidn::Format::Float3, g_Config->wholeWidth, g_Config->screenHeight);
    filter.setImage("output", outputFrameVec.data(), oidn::Format::Float3, g_Config->wholeWidth, g_Config->screenHeight);
    filter.set("hdr", false);
    filter.commit();

    filter.execute();

    // Check for errors
    const char* errorMessage;
    if (device.getError(errorMessage) != oidn::Error::None)
    {
        std::cout << "Error2: " << errorMessage << std::endl;
    }

    glBindTexture(GL_TEXTURE_2D, curOutputTexId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, g_Config->wholeWidth, g_Config->screenHeight, 0, GL_RGB, GL_FLOAT, outputFrameVec.data());
}

IDenoisePass *CreateDenoisePass()
{
    IDenoisePass* denoisePass = nullptr;
    switch (g_Config->curDenoise)
    {
    case DenoiseType::ODIN:
        denoisePass = new OIDNDenoisePass();
        break;
    default:
        break;
    }
    return denoisePass;
}

