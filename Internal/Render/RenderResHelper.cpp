#include "RenderResHelper.h"

void RenderResHelper::generateFBO()
{
    
}

void RenderResHelper::deleteFBO()
{
}

GPUTexture RenderResHelper::generateGPUTexture(TextureInfo &textureInfo)
{
    GPUTexture gpuTexture;
    glGenTextures(1, &gpuTexture.texId);
    if(textureInfo.texType == TextureType::Buffer)
    {
        glGenBuffers(1, &gpuTexture.texBufferId);
        glBindBuffer(GL_TEXTURE_BUFFER, gpuTexture.texBufferId);
        glBufferData(GL_TEXTURE_BUFFER, textureInfo.dataSize, textureInfo.data, GL_STATIC_DRAW);
        glBindTexture(GL_TEXTURE_BUFFER, gpuTexture.texId);
        glTexBuffer(GL_TEXTURE_BUFFER, textureInfo.internalFormat, gpuTexture.texBufferId);
    }
    else if(textureInfo.texType == TextureType::Image2D)
    {
        glBindTexture(GL_TEXTURE_2D, gpuTexture.texId);
        glTexImage2D(GL_TEXTURE_2D, 0, textureInfo.internalFormat, textureInfo.width, textureInfo.height, 0, textureInfo.format, textureInfo.type, textureInfo.data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, textureInfo.magFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureInfo.minFilter);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    else if(textureInfo.texType == TextureType::Image3DTextureArray2D)
    {
        glBindTexture(GL_TEXTURE_2D_ARRAY, gpuTexture.texId);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, textureInfo.internalFormat, textureInfo.width, textureInfo.height, textureInfo.dataSize, 0, textureInfo.format, textureInfo.type, textureInfo.data);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, textureInfo.magFilter);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, textureInfo.minFilter);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    }
    return gpuTexture;
}

void RenderResHelper::destroyGPUTexture(GPUTexture &gpuTexture)
{
    if(gpuTexture.texId != 0)
    {
        glDeleteTextures(1, &gpuTexture.texId);
    }
    if(gpuTexture.texBufferId != 0)
    {
        glDeleteBuffers(1, &gpuTexture.texBufferId);
    }
}

void checkGLError()
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) 
    {
        std::cerr << "OpenGL error: " << error << std::endl;
    }
}

