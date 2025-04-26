#version 430 core

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in float matID;    // 这里用int有错误

layout(location = 0) out vec3 Position;
layout(location = 1) out vec3 Normal;
layout(location = 2) out vec2 TexCoord;
layout(location = 3) flat out int MatID;
layout(location = 4) out mat3 TBN;

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

vec3 GetPerpVector(vec3 u)
{
    vec3 a = abs(u);
    uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
    uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
    uint zm = 1 ^ (xm | ym);
    return cross(u, vec3(xm, ym, zm));
}

void main()
{
    vec4 r1 = texelFetch(transformTex, ivec2((gl_InstanceID + instanceBase) * 4 + 0, 0), 0);
    vec4 r2 = texelFetch(transformTex, ivec2((gl_InstanceID + instanceBase) * 4 + 1, 0), 0);
    vec4 r3 = texelFetch(transformTex, ivec2((gl_InstanceID + instanceBase) * 4 + 2, 0), 0);
    vec4 r4 = texelFetch(transformTex, ivec2((gl_InstanceID + instanceBase) * 4 + 3, 0), 0);
    mat4 instanceModelMat = mat4(r1, r2, r3, r4);

    vec4 worldPos = instanceModelMat * vec4(vertex, 1.0f);
    gl_Position = projectionView * worldPos;
    Position = worldPos.xyz;

    Normal = normalize(transpose(inverse(mat3(instanceModelMat))) * normal);
    vec3 Bitangent = normalize(GetPerpVector(normal));
    vec3 Tangent = normalize(cross(Bitangent, Normal));
    TBN = mat3(Tangent, Bitangent, Normal);

    TexCoord = uv;
    MatID = floatBitsToInt(matID);
}