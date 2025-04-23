#version 330 core
out vec4 FragColor;
uniform vec4 color;

void main()
{
    gl_FragColor = color;
}