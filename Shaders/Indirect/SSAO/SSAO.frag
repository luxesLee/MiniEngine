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
uniform float ssaoRadius;
uniform float ssaoBias;
uniform vec3 samples[64];

void main() 
{
    vec2 texCoord = (gl_FragCoord.xy) / screenAndInvScreen.xy;
    vec2 NoiseScale = vec2(screenAndInvScreen.xy / 4.0f);

    vec3 ViewPosition = texture(GBuffer0, texCoord).xyz;
    vec3 ViewNormal = normalize(texture(GBuffer1, texCoord).xyz);
    vec3 randomVec = normalize(texture(NoiseTex, texCoord * NoiseScale).xyz);
    vec3 tangent = normalize(randomVec - ViewNormal * dot(randomVec, ViewNormal));
    vec3 bitangent = cross(ViewNormal, tangent);
    mat3 TBN = mat3(tangent, bitangent, ViewNormal);
    
    float factor = 0.0f;
    for(int i = 0; i < ssaoKernel; i++)
    {
        vec3 samplePos = ViewPosition + TBN * samples[i] * ssaoRadius;

        vec4 offset = projection * vec4(samplePos, 1.0f);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5f;

        float sampleDepth = texture(GBuffer0, offset.xy).z;

        float rangeCheck = smoothstep(0.0, 1.0, ssaoRadius / abs(ViewPosition.z - sampleDepth));
        factor += (sampleDepth >= ViewPosition.z + ssaoBias ? 1.0 : 0.0) * rangeCheck;
    }
    factor = 1.0 - factor / ssaoKernel;
    ssaoFactor = factor;
}