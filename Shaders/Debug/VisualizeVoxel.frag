#version 430 core

in vec4 voxelColor;

out vec4 FragColor;

void main()
{
    FragColor = voxelColor;
}