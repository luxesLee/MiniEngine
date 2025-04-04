#pragma once
#include <memory>
#include "Util/Singleton.h"
#include "PresentWindow.h"
#include "ImGuiManager.h"
#include "Engine.h"

class Engine;

class App : public Singleton<App>
{
    friend class Singleton<App>;
public:
    void Init();
    void Destroy();
    void Run();

private:
    App() = default;
    ~App() = default;

    void BindImGUICallbackFunction();

private:
    std::unique_ptr<ImGuiManager> imguiManager = nullptr;
    std::unique_ptr<PresentWindow> window = nullptr;
    std::unique_ptr<Engine> engine = nullptr;

};
