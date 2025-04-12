#include "App.h"
#include <GLFW/glfw3.h>
#include "Config.h"
#include "Render/ShaderManager.h"
#include "Shader.h"
#include "Util/ConfigLoader.h"
#include "Camera.h"

void App::Init()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = std::make_unique<PresentWindow>(g_Config->wholeWidth, g_Config->screenHeight, g_Config->title);
    imguiManager = std::make_unique<ImGuiManager>(window->getWindow());
    engine = std::make_unique<Engine>();

    ReloadConfig();
}

void App::Destroy()
{
    imguiManager.reset();
    window.reset();

    glfwTerminate();
}

void App::Run()
{
    while(window && !window->ShouldClose())
    {
        window->Update();
        engine->Update();

        engine->Render();
        imguiManager->Render();
    }
}

void App::ReloadConfig()
{
    SceneConfig config;
    ParseConfig(g_Config->configPath, config);

    engine->UpdateScene(config);
    
    g_Camera->UpdateCameraParamters(config.cameraConfig.position, config.cameraConfig.lookAt, config.cameraConfig.zoom);
    g_Config->cameraZFar = config.cameraConfig.zNear;
    g_Config->cameraZNear = config.cameraConfig.zFar;
}
