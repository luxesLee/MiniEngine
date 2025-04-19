#version 430 core

out float ssaoFactor;

layout(binding = 0) uniform sampler2D GBuffer0; // Position
layout(binding = 1) uniform sampler2D GBuffer1; // Normal
layout(binding = 2) uniform sampler2D NoiseTex;

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

uniform int ssaoKernel;
uniform vec3 samples[64];

float radius = 0.5;
float bias = 0.0025;

void main() 
{
    vec2 texCoord = vec2(gl_FragCoord.x / screenAndInvScreen.x, gl_FragCoord.y / screenAndInvScreen.y);
    vec3 WorldPosition = texture(GBuffer0, texCoord).xyz;
    vec3 Normal = normalize(texture(GBuffer1, texCoord).xyz);
    // 引入随机旋转向量
    vec2 NoiseScale = vec2(screenAndInvScreen.xy / 4.0f);
    vec3 randomVec = normalize(texture(NoiseTex, texCoord * NoiseScale).xyz);
    vec3 tangent = normalize(randomVec - Normal * dot(randomVec, Normal));
    vec3 bitangent = cross(Normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, Normal);

    float factor = 0.0f;
    for(int i = 0; i < ssaoKernel; i++)
    {
        // 由切线空间到世界空间
        vec3 samplePos = WorldPosition + TBN * samples[i] * radius;

        // 世界空间到屏幕空间
        vec4 offset = projectionView * vec4(samplePos, 1.0f);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float sampleDepth = texture(GBuffer0, offset.xy).z;
        // 过滤掉物体边缘：当前着色点与周围采样点的深度差异越大，rangeCheck越接近0
        float rangeCheck = smoothstep(0.0f, 1.0f, radius / abs(WorldPosition.z - sampleDepth));
        factor += (sampleDepth <= samplePos.z + bias ? 1.0 : 0.0);
    }
    factor = 1.0 - factor / ssaoKernel;
    ssaoFactor = factor;
}