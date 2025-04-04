#version 460 core

void main()
{
    int v = gl_VertexID % 3;
    float x = -1.0 + float((v & 1) << 2);
    float y = -1.0 + float((v & 2) << 1);
    gl_Position = vec4(x, y, 0.0f, 1.0f);
}