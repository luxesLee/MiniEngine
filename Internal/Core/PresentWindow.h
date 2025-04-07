#pragma once

#include <string>

class GLFWwindow;

class PresentWindow
{
public:
    PresentWindow(uint16_t width, uint16_t height, std::string title);
    ~PresentWindow();
    void Update();
    bool ShouldClose();
    GLFWwindow* getWindow() {return window;}
    

private:
    void InitWindow(uint16_t width, uint16_t height, std::string title);
    void processInput();
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
    static void mousebutton_callback(GLFWwindow* window, int button, int action, int mods);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

private:
    GLFWwindow* window;
    uint16_t curWidth;
    uint16_t curHeight;
    bool bResize;
};