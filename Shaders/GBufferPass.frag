#version 430 core

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TexCoord;
layout(location = 3) flat in int MatID;
layout(location = 4) in mat3 TBN;

// GBuffer0 WorldPos(RGB8)|AO(A8)
// GBuffer1 WorldNormal(RGB8)
// GBuffer2 specularTint|metallic|roughness
// GBuffer3 BaseColor(RGB8)
// GBuffer4 Emission(RGB8)
layout(location = 0) out vec4 GBuffer0;
layout(location = 1) out vec4 GBuffer1;
layout(location = 2) out vec4 GBuffer2;
layout(location = 3) out vec4 GBuffer3;
layout(location = 4) out vec4 GBuffer4;

layout(binding = 1) uniform sampler2D matTex;
layout(binding = 2)uniform sampler2DArray textureMapsArrayTex;

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

    int baseColorTexId;
    int metallicRoughnessTexID;
    int normalmapTexID;
    int emissionmapTexID;

    float opacity;
    float alphaMode;
    float alphaCutoff;
    float padding2;
};

Material GetMatrixData(int matID, vec2 uv)
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

    ivec4 texIDs            = ivec4(param7);

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

    if(texIDs.z >= 0)
    {
        vec3 normal = texture(textureMapsArrayTex, vec3(uv, texIDs.z)).rgb;
        normal = normalize(normal * 2.0 - 1.0);
        GBuffer1.rgb = normalize(TBN * normal);
    }

    if(texIDs.w >= 0)
    {
        data.emission = pow(texture(textureMapsArrayTex, vec3(uv, texIDs.w)).rgb, vec3(2.2));
    }

    return data;
}

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
    GBuffer0.rgb = Position;
    GBuffer1.rgb = Normal;

    Material mat = GetMatrixData(MatID, TexCoord);
    GBuffer2.x = mat.specularTint;
    GBuffer2.y = mat.metallic;
    GBuffer2.z = mat.roughness;

    GBuffer3.rgb = mat.baseColor;
    GBuffer3.a = MatID;
    
    GBuffer4.rgb = mat.emission;
}
