#version 450 core

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in float matID;

layout(location = 0) out vec3 Normal;
layout(location = 1) out vec2 TexCoords;
layout(location = 2) flat out int MatID;

layout(binding = 0)uniform sampler2D transformTex;

uniform int instanceBase;

void main()
{
    vec4 r1 = texelFetch(transformTex, ivec2((gl_InstanceID + instanceBase) * 4 + 0, 0), 0);
    vec4 r2 = texelFetch(transformTex, ivec2((gl_InstanceID + instanceBase) * 4 + 1, 0), 0);
    vec4 r3 = texelFetch(transformTex, ivec2((gl_InstanceID + instanceBase) * 4 + 2, 0), 0);
    vec4 r4 = texelFetch(transformTex, ivec2((gl_InstanceID + instanceBase) * 4 + 3, 0), 0);
    mat4 instanceModelMat = mat4(r1, r2, r3, r4);

    gl_Position = instanceModelMat * vec4(vertex, 1.0f);
    Normal = normalize(transpose(inverse(mat3(instanceModelMat))) * normal);
    TexCoords = uv;
    MatID = floatBitsToInt(matID);
}