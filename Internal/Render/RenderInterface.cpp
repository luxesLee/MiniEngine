#include "RenderInterface.h"
#include "ShaderManager.h"
#include "Core/Shader.h"
#include "mat4x4.hpp"
#include "gtc/matrix_transform.hpp"
#include <iostream>

void checkGLError()
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) 
    {
        std::cerr << "OpenGL error: " << error << std::endl;
    }
}

GLuint generateFBO()
{
    GLuint fboID;
    glGenFramebuffers(1, &fboID);
    return fboID;
}

void deleteFBO(GLuint fboId)
{
    if(fboId != 0)
    {
        glDeleteFramebuffers(1, &fboId);
    }
}

GPUTexture generateTexture(const TextureDesc& desc)
{
    GPUTexture gpuTex;
    glGenTextures(1, &gpuTex.texId);
    glBindTexture(desc.type, gpuTex.texId);

    switch (desc.type)
    {
    case TextureType::Buffer:
        {
            glGenBuffers(1, &gpuTex.texBufferId);
            glBindBuffer(GL_TEXTURE_BUFFER, gpuTex.texBufferId);
            glBufferData(GL_TEXTURE_BUFFER, desc.width, desc.data, GL_STATIC_DRAW);
            glTexBuffer(GL_TEXTURE_BUFFER, desc.format, gpuTex.texBufferId);
        }
        break;
    case TextureType::TEXTURE_2D:
        glTexImage2D(GL_TEXTURE_2D, desc.mipLevel, desc.format, desc.width, desc.height, 0, desc.dataFormat, desc.dataType, desc.data);
        break;
    case TextureType::TEXTURE_2D_ARRAY:
        glTexImage3D(GL_TEXTURE_2D_ARRAY, desc.mipLevel, desc.format, desc.width, desc.height, desc.depth, 0, desc.dataFormat, desc.dataType, desc.data);
        break;
    case TextureType::TEXTURE_3D:
        glTexStorage3D(GL_TEXTURE_3D, desc.mipLevel, desc.format, desc.width, desc.height, desc.depth);
        break;
    case TextureType::TEXTURE_CUBE_MAP:
        {
            for(Int i = 0; i < 6; i++)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, desc.mipLevel, desc.format, desc.width, desc.height, 0, desc.dataFormat, desc.dataType, desc.data);
            }
        }
        break;
    default:
        break;
    }

    if(desc.type != TextureType::Buffer)
    {
        glTexParameteri(desc.type, GL_TEXTURE_WRAP_S, desc.samplerDesc.warpS);
        glTexParameteri(desc.type, GL_TEXTURE_WRAP_T, desc.samplerDesc.warpT);
        glTexParameteri(desc.type, GL_TEXTURE_WRAP_R, desc.samplerDesc.warpR);
        glTexParameteri(desc.type, GL_TEXTURE_MAG_FILTER, desc.samplerDesc.filterMag);
        glTexParameteri(desc.type, GL_TEXTURE_MIN_FILTER, desc.samplerDesc.filterMin);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (desc.samplerDesc.borderColor == Bordercolor::BLACK 
                        ? BLACK_BORDER : WHITE_BORDER));
    }

    glBindTexture(desc.type, 0);
    return gpuTex;
}

void deleteTexture(GPUTexture tex)
{
    if(tex.texId != 0)
    {
        glDeleteTextures(1, &tex.texId);
    }
    if(tex.texBufferId != 0)
    {
        glDeleteBuffers(1, &tex.texBufferId);
    }
}

GLuint generateVAO()
{
    GLuint vao;
    glGenVertexArrays(1, &vao);
    return vao;
}

void deleteVAO(GLuint VAO)
{
    glDeleteVertexArrays(1, &VAO);
}

unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

GPUTexture RenderResHelper::generateCubeEnvMap(const std::vector<Texture> &envMap)
{
    GPUTexture gpuTexture;
    glGenTextures(1, &gpuTexture.texId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, gpuTexture.texId);
    for(Int i = 0; i < envMap.size(); i++)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, envMap[i].width, envMap[i].height, 0, GL_RGB, GL_UNSIGNED_BYTE, envMap[i].data.data());
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return gpuTexture;
}

GPUTexture generateIrradianceMap(const GPUTexture& gpuTex)
{
    GLuint fboTmp = generateFBO();
    glBindFramebuffer(GL_FRAMEBUFFER, fboTmp);

    TextureDesc irraDesc{32, 32, 0, TextureType::TEXTURE_CUBE_MAP, TextureFormat::RGB, 
                        LINEAR_CLAMP_TO_EDGE_BLACK_BORDER_SAMPLER, 0, nullptr, DataFormat::DataFormat_RGB, DataType::UNSIGNED_BYTE};
    GPUTexture irradianceTex = generateTexture(irraDesc);

    static glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    static glm::mat4 captureViews[] =
    {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    Shader* shaderIrradianceConv = g_ShaderManager.GetShader("IrradianceConv");
    if(!shaderIrradianceConv)
    {
        return irradianceTex;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, gpuTex.texId);
    glViewport(0, 0, 32, 32);
    shaderIrradianceConv->use();
    shaderIrradianceConv->setMat4("projection", captureProjection);
    for(Int i = 0; i < 6; i++)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceTex.texId, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shaderIrradianceConv->setMat4("view", captureViews[i]);

        renderCube();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    deleteFBO(fboTmp);

    return irradianceTex;
}
