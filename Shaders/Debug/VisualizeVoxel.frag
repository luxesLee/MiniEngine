#version 430 core

out vec4 color;

layout(binding = 0) uniform sampler3D voxelTex;
layout(binding = 1) uniform sampler2D posTex;

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

uniform mat4 VoxelProjection;
uniform int VoxelSize;

void main()
{
    vec3 ViewPosition = texelFetch(posTex, ivec2(gl_FragCoord.xy), 0).xyz;
    vec3 worldPosition = (inverse(view) * vec4(ViewPosition, 1.0f)).xyz;
    vec4 voxelNDC = VoxelProjection * vec4(worldPosition, 1.0f);
    ivec3 texCoord3D = ivec3((voxelNDC.xyz + vec3(1)) * 0.5f * VoxelSize);
    vec4 val = texelFetch(voxelTex, texCoord3D, 0);
    color = val;
}