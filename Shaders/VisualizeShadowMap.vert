#version 430 core
layout (location = 0) in vec3 aPos;

layout(binding = 0) uniform CommonUBO
{
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 projectionView;
    mat4 invProjection;
    mat4 invViewProjection;
    vec4 screenAndInvScreen;
    vec4 cameraPosition;
    int lightNum;
};

void main()
{
    gl_Position = projectionView * vec4(aPos, 1.0);
}
