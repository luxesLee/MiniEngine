#version 430 core

out vec4 color;

uniform sampler2D imgTex;
// uniform sampler3D LUT;


// //https://github.com/h3r2tic/tony-mc-mapface/blob/main/shader/tony_mc_mapface.hlsl
// vec3 TonyMcMapface(vec3 color) 
// {
//     vec3 encoded = color / (color + 1.0);
//     const float LUT_DIMS = 48.0;
//     const float3 uv = encoded * ((LUT_DIMS - 1.0) / LUT_DIMS) + 0.5 / LUT_DIMS;
//     float3 result = LUT.SampleLevel(LinearClampSampler, uv, 0);
// 	result = LinearToSRGB(result);
// 	return result;
// }

#define Gamma 2.2
#define INVGamma 0.45454545
vec3 LinearToSRGB(vec3 linearRGB)
{
    return pow(linearRGB, vec3(INVGamma));
}

vec3 LinearToneMapping(vec3 color)
{
    color = clamp(color, 0.0, 1.0f);
    return LinearToSRGB(color);
}

void main()
{
    color = vec4(LinearToSRGB(texelFetch(imgTex, ivec2(gl_FragCoord.xy), 0).rgb), 1.0f);
}