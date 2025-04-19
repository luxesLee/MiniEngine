#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include "PresentWindow.h"
#include "Camera.h"
#include "Util/Timer.h"
#include "Config.h"
#include "Render/RenderResHelper.h"

PresentWindow::PresentWindow(uint16_t width, uint16_t height, std::string title)
    : curWidth(width), curHeight(height), bResize(false)
{
    InitWindow(width, height, title);
}

PresentWindow::~PresentWindow()
{
    if(window)
    {
        glfwDestroyWindow(window);
    }
}

void PresentWindow::Update()
{
    processInput();
    glfwPollEvents();
    glfwSwapBuffers(window);
}

bool PresentWindow::ShouldClose()
{
    return glfwWindowShouldClose(window);
}

void PresentWindow::InitWindow(uint16_t width, uint16_t height, std::string title)
{
    window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
    g_Camera->screenWidth = width;
    g_Camera->screenHeight = height;
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mousebutton_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) 
    {
        std::cout << "glad failed" << std::endl;
    }
    glfwSwapInterval(0);
}

void PresentWindow::processInput()
{
    static Timer timer;
    float dt = timer.MarkInSeconds();

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        g_Camera->ProcessKeyboard(FORWARD, dt);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        g_Camera->ProcessKeyboard(BACKWARD, dt);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        g_Camera->ProcessKeyboard(LEFT, dt);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        g_Camera->ProcessKeyboard(RIGHT, dt);
}

void PresentWindow::framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    g_Config->wholeWidth = width;
    g_Camera->screenWidth = width - g_Config->imguiWidth;
    g_Camera->screenHeight = height;
    g_Camera->bResize = true;
    g_Config->screenWidth = width - g_Config->imguiWidth;
    g_Config->screenHeight = height;
    g_Config->accumulateFrames = 0;
}

float lastX = g_Config->screenWidth / 2.0f;
float lastY = g_Config->screenHeight / 2.0f;
bool firstMouse = true;
void PresentWindow::mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if(!g_Config->bMouseMove)
    {
        return;
    }

    g_Config->accumulateFrames = 0;
    g_Camera->ProcessMouseMovement(xoffset, yoffset);
}

void PresentWindow::mousebutton_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) 
    {
        if (action == GLFW_PRESS) 
        {
            g_Config->bMouseMove = true;
        } 
        else if (action == GLFW_RELEASE) 
        {
            g_Config->bMouseMove = false;
        }
    }
}

void PresentWindow::scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    g_Camera->ProcessMouseScroll(static_cast<float>(yoffset));
    g_Config->accumulateFrames = 0;
}
