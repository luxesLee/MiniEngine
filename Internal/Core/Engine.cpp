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
    renderer->Update(scene.get());    
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

    for(const auto& light : config.lightConfigs)
    {
        scene->AppendLightMesh(light);
        scene->AppendLight(Light(light));
    }

    if(config.envMapConfig.bCubeMap)
    {
        modelLoader->loadEnvMap(scene.get(), config.envMapConfig.envMapPaths);
    }

    scene->BuildScene();
    scene->InitShadowMapFBO();
    renderer->InitRenderResource();
}

void Engine::ReLoad()
{
    
}
