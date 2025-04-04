#pragma once
#include <entt.hpp>
#include <memory>
#include "Util/ModelLoader.h"
#include "Render/Renderer.h"
#include "Scene.h"

class Engine
{
public:
    Engine();
    ~Engine();

    void Update();
    void Resize();

private:
    void InitializeModel();

private:
    entt::registry reg;
    std::unique_ptr<ModelLoader> modelLoader;
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<Scene> scene;
    // todo: physical engine
};
