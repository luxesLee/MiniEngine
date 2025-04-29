#pragma once
#include <glad/glad.h>
#include "Texture.h"
#include "Util/Types.h"

void checkGLError();

GLuint generateFBO();
void deleteFBO(GLuint fboId);

GPUTexture generateTexture(const TextureDesc& desc);
void deleteTexture(GPUTexture tex);
GPUTexture generateIrradianceMap(const GPUTexture& gpuTex);


GLuint generateVAO();
void deleteVAO(GLuint VAO);






class RenderResHelper
{
public:
    static GPUTexture generateCubeEnvMap(const std::vector<Texture>& envMap);
};

