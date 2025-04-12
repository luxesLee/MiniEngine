#include "Engine.h"
#include "Camera.h"
#include "Render/ShaderManager.h"
#include "Util/Timer.h"

Engine::Engine()
{
    modelLoader = std::make_unique<ModelLoader>(reg);
    renderer = std::make_unique<Renderer>();
    scene = std::make_unique<Scene>();
}

Engine::~Engine()
{
    modelLoader.reset();
    scene->CleanScene();
}

void Engine::Update()
{
    Resize();
    renderer->Update();    
}

void Engine::Render()
{
    renderer->Render(scene.get());
}

void Engine::Resize()
{
    if(g_Camera->bResize)
    {
        renderer->Resize();
        scene->DeleteFBO();
        scene->InitFBO();
        g_Camera->bResize = false;
    }
}

void Engine::UpdateScene(const SceneConfig& config)
{
    scene->CleanScene();

    for(const auto& modelConfig : config.modelConfigs)
    {
        modelLoader->loadModel(scene.get(), modelConfig, config.matConfigMap);
    }

    for(auto& light : config.lightConfigs)
    {
        scene->AppendLight(Light(light));
    }

    if(config.envMapConfig.envMapPath.size() > 0)
    {
        modelLoader->loadEnvMap(scene.get(), config.envMapConfig.envMapPath);
    }

    scene->BuildScene();
    scene->InitFBO();
}
