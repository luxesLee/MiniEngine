#include "Engine.h"
#include "Camera.h"
#include "Render/ShaderManager.h"
#include "Util/Timer.h"

Engine::Engine()
{
    modelLoader = std::make_unique<ModelLoader>(reg);
    renderer = std::make_unique<Renderer>(g_Camera->screenWidth, g_Camera->screenHeight);
    scene = std::make_unique<Scene>();
    InitializeModel();
}

Engine::~Engine()
{
    modelLoader.reset();
}

void Engine::Update()
{
    if(scene->getSceneDirty())
    {
        InitializeModel();
    }

    Resize();

    static Timer timer;
    float dt = timer.MarkInSeconds();
    renderer->Update();
    renderer->Render(scene.get());
}

void Engine::Resize()
{
    if(g_Camera->bResize)
    {
        renderer->Resize(g_Camera->screenWidth, g_Camera->screenHeight);
        g_Camera->bResize = false;
    }
}

void Engine::InitializeModel()
{
    modelLoader->loadModel(scene.get(), g_Config->initModelPath);
    modelLoader->loadLight(scene.get(), "");
    scene->buildScene();
}
