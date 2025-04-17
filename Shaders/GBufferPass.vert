#version 430 core

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in float matID;    // 这里用int有错误

layout(location = 0) out vec3 WorldPosition;
layout(location = 1) out vec3 WorldNormal;
layout(location = 2) out vec2 TexCoord;
layout(location = 3) flat out int MatID;

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
    gl_Position = projectionView * worldPos;
    WorldPosition = worldPos.xyz;
    WorldNormal = normalize(transpose(inverse(mat3(instanceModelMat))) * normal);
    TexCoord = uv;
    MatID = floatBitsToInt(matID);
}