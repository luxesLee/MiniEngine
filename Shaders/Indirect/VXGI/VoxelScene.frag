#version 450 core
#extension GL_ARB_shader_image_load_store : require
// #extension GL_ARB_shading_language_include : require

layout(r32ui, binding = 0) uniform volatile coherent uimage3D colorVoxel;
layout(r32ui, binding = 1) uniform volatile coherent uimage3D normalVoxel;

layout(binding = 1) uniform sampler2D matTex;
layout(binding = 2) uniform sampler2DArray textureMapsArrayTex;

in GS_OUT {
    vec3 VoxelPosition;
    vec3 Normal;
    vec2 TexCoords;
    flat int MatID;
} fs_in;

struct Medium
{
    int type;
    float density;
    vec3 color;
    float anisotropy;
};

struct Material
{
    vec3 baseColor;
    float anisotropic;

    vec3 emission;
    float padding1;

    float metallic;
    float roughness;
    float subsurface;
    float specularTint;

    float sheen;
    float sheenTint;
    float clearcoat;
    float clearcoatRoughness;

    float specTrans;
    float ior;
    // float mediumType;
    // float mediumDensity;
    
    // vec3 mediumColor;
    // float mediumAnisotropy;
    Medium medium;

    float baseColorTexId;
    float metallicRoughnessTexID;
    int normalmapTexID;
    float emissionmapTexID;

    float opacity;
    float alphaMode;
    float alphaCutoff;
    float padding2;
};

Material GetMatrixData(int matID, vec2 uv, inout vec3 normal)
{
    int matIndex = 8 * matID;

    vec4 param1 = texelFetch(matTex, ivec2(matIndex + 0, 0), 0);
    vec4 param2 = texelFetch(matTex, ivec2(matIndex + 1, 0), 0);
    vec4 param3 = texelFetch(matTex, ivec2(matIndex + 2, 0), 0);
    vec4 param4 = texelFetch(matTex, ivec2(matIndex + 3, 0), 0);
    vec4 param5 = texelFetch(matTex, ivec2(matIndex + 4, 0), 0);
    vec4 param6 = texelFetch(matTex, ivec2(matIndex + 5, 0), 0);
    vec4 param7 = texelFetch(matTex, ivec2(matIndex + 6, 0), 0);
    vec4 param8 = texelFetch(matTex, ivec2(matIndex + 7, 0), 0);

    Material data;
    data.baseColor          = param1.rgb;
    data.anisotropic        = param1.w;

    data.emission           = param2.rgb;

    data.metallic           = param3.x;
    data.roughness          = max(param3.y, 0.001);
    data.subsurface         = param3.z;
    data.specularTint       = param3.w;

    data.sheen              = param4.x;
    data.sheenTint          = param4.y;
    data.clearcoat          = param4.z;
    data.clearcoatRoughness = mix(0.1f, 0.001f, param4.w); // Remapping from gloss to roughness

    data.specTrans          = param5.x;
    data.ior                = param5.y;
    data.medium.type        = int(param5.z);
    data.medium.density     = param5.w;

    data.medium.color       = param6.rgb;
    data.medium.anisotropy  = clamp(param6.w, -0.9, 0.9);

    ivec4 texIDs           = ivec4(param7);

    data.opacity            = param8.x;
    data.alphaMode          = int(param8.y);
    data.alphaCutoff        = param8.z;

    if(texIDs.x >= 0)
    {
        vec4 col = texture(textureMapsArrayTex, vec3(uv, texIDs.x));
        data.baseColor.rgb *= pow(col.rgb, vec3(2.2));
        data.opacity *= col.a;
    }

    if(texIDs.y >= 0)
    {
        vec2 matRgh = texture(textureMapsArrayTex, vec3(uv, texIDs.y)).bg;
        data.metallic = matRgh.x;
        data.roughness = max(matRgh.y, 0.001);
    }

    // // if(texIDs.z >= 0)
    // // {
    // //     vec3 texNormal = texture(textureMapsArrayTex, vec3(uv, texIDs.z)).rgb;
    // //     texNormal = normalize(texNormal * 2.0 - 1.0);
    // // }

    if(texIDs.w >= 0)
    {
        data.emission = pow(texture(textureMapsArrayTex, vec3(uv, texIDs.w)).rgb, vec3(2.2));
    }

    return data;
}

vec3 EncodeNormal(vec3 normal)
{
    return normal * 0.5 + vec3(0.5f);
}

vec4 convRGBA8ToVec4(uint val){
    return vec4(float((val & 0x000000FF)), float ((val & 0x0000FF00) >> 8U), float (( val & 0x00FF0000) >> 16U), float ((val & 0xFF000000) >> 24U));
}

uint convVec4ToRGBA8(vec4 val){
    return (uint(val.w) & 0x000000FF) << 24U | (uint(val.z) &0x000000FF) << 16U | (uint(val.y) & 0x000000FF) << 8U | (uint(val.x) & 0x000000FF);
}

void imageAtomicRGBA8Avg(layout(r32ui) volatile coherent uimage3D imgUI, ivec3 coords, vec4 val)
{
    val.xyz *= 255.0f;
    uint uIntVal = convVec4ToRGBA8(val);
    uint prevVal = 0;
    uint curVal;
    uint iter = 0;
    // 第一次时prevVal = 0，直接存入
    // 后续必进入循环体，与imgUI中保存的数据叠加
    while((curVal = imageAtomicCompSwap(imgUI, coords, prevVal, uIntVal)) != prevVal && iter < 255)
    {
        prevVal = curVal;
        vec4 rval = convRGBA8ToVec4(curVal);
        rval.xyz *= rval.w;
        vec4 curValF = rval + val;
        curValF.xyz /= curValF.w;
        uIntVal = convVec4ToRGBA8(curValF);
        iter++;
    }
}

void main()
{
    vec3 normal = fs_in.Normal;
    Material mat = GetMatrixData(fs_in.MatID, fs_in.TexCoords, normal);

    vec3 color = mat.baseColor;
    imageAtomicRGBA8Avg(colorVoxel, ivec3(fs_in.VoxelPosition), vec4(color, 1.0f));
    imageAtomicRGBA8Avg(normalVoxel, ivec3(fs_in.VoxelPosition), vec4(EncodeNormal(normal), 1.0f));
}