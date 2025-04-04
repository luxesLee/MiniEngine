#include "App.h"
#include <GLFW/glfw3.h>
#include "Render/ShaderManager.h"
#include "Shader.h"

void App::Init()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = std::make_unique<PresentWindow>();
    imguiManager = std::make_unique<ImGuiManager>(window->getWindow());
    engine = std::make_unique<Engine>();

    BindImGUICallbackFunction();
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
        imguiManager->Render();
    }
}

void App::BindImGUICallbackFunction()
{
    imguiManager->setCaptureFrameFunc([&]()->void
    {

    });
}
