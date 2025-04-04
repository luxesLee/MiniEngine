#pragma once
#include <glad/glad.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <algorithm>
#include "Config.h"

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Default camera values
const float SENSITIVITY =  0.1f;

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
    // camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;

    // camera options
    float Aspect;
    uint32_t screenWidth;
    uint32_t screenHeight;
    bool bResize;

    static Camera* getInstance()
    {
        static Camera instance(glm::vec3(7.72f, 2.5f, -1.0f), glm::vec3(6.72f, 2.5f, -1.0f), 45.0f);
        return &instance;
    }

private:
    glm::vec3 WorldUp = glm::vec3(0, 1, 0);
    float Zoom;
    float pitch;
    float yaw;

private:
    // constructor with vectors
    Camera(glm::vec3 _position, glm::vec3 lookat, double _zoom) 
        : Position(_position), Zoom(_zoom), bResize(false)
    {
        glm::vec3 dir = lookat - Position;
        pitch = glm::degrees(asin(dir.y));
        yaw = glm::degrees(atan2(dir.z, dir.x));

        updateCameraVectors();
    }

    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;

public:
    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    glm::mat4 GetProjectionMatrix()
    {
        return glm::perspective(glm::radians(Zoom), screenWidth * 1.0f / screenHeight, g_Config->cameraZNear, g_Config->cameraZFar);
    }

    glm::vec4 GetScreenAndInvScreen()
    {
        return glm::vec4(screenWidth, screenHeight, 1.0 / screenWidth, 1.0 / screenHeight);
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = g_Config->cameraMoveSpeed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= g_Config->cameraRotSensitivity;
        yoffset *= g_Config->cameraRotSensitivity;

        yaw   += xoffset;
        pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
        }

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;
    }

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        Front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        Front.y = sin(glm::radians(pitch));
        Front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        Front = glm::normalize(Front);

        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};

#define g_Camera Camera::getInstance()