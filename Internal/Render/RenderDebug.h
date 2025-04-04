#pragma once
#include "renderdoc_app.h"


class RenderDebugger 
{
public:
    static void startFrameCapture(RENDERDOC_DevicePointer device = nullptr);
    
    static void endFrameCapture(RENDERDOC_DevicePointer device = nullptr);

private:
    static RENDERDOC_API_1_0_0* getRenderDocApi();

private:
    static RENDERDOC_API_1_0_0* rdoc_;
};