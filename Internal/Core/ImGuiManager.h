#pragma once
#include <functional>
class GLFWwindow;

class ImGuiManager
{
public:
    explicit ImGuiManager(GLFWwindow* window);
    ~ImGuiManager();
    void Render();

    void setCaptureFrameFunc(const std::function<void(void)>& func)
    {
        captureFrameFunc = func;
    }

private:
    void RenderMainMenuBar();
    void RenderOpenDialog();

    void RenderSettingWindow();


private:
    std::function<void(void)> captureFrameFunc;
};

