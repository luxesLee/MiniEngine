#version 430 core

out vec4 GBuffer0;

layout(binding = 0) uniform sampler2D ssao;

void main() 
{
    vec2 texelSize = 1.0 / vec2(textureSize(ssao, 0));
    vec2 texCoord = gl_FragCoord.xy * texelSize;

    float result = 0.0f;
    for(int x = -2; x < 2; x++)
    {
        for(int y = -2; y < 2; y++)
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(ssao, texCoord + offset).r;
        }
    }
    GBuffer0.a = result / 16.0;
}