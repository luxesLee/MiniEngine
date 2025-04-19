#version 430 core

layout(location = 0) in vec3 vertex;

layout(binding = 0)uniform sampler2D transformTex;

uniform int instanceBase;

void main()
{
    vec4 r1 = texelFetch(transformTex, ivec2((gl_InstanceID + instanceBase) * 4 + 0, 0), 0);
    vec4 r2 = texelFetch(transformTex, ivec2((gl_InstanceID + instanceBase) * 4 + 1, 0), 0);
    vec4 r3 = texelFetch(transformTex, ivec2((gl_InstanceID + instanceBase) * 4 + 2, 0), 0);
    vec4 r4 = texelFetch(transformTex, ivec2((gl_InstanceID + instanceBase) * 4 + 3, 0), 0);
    mat4 instanceModelMat = mat4(r1, r2, r3, r4);

    vec4 worldPos = instanceModelMat * vec4(vertex, 1.0f);
    gl_Position = worldPos;
}