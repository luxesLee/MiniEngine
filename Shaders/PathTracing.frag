#version 430 core

out vec4 color;
out vec4 accum;

layout(binding = 0)uniform sampler2D accumTex;
layout(binding = 1)uniform samplerBuffer vertTex;
layout(binding = 2)uniform isamplerBuffer indiceTex;
layout(binding = 3)uniform samplerBuffer normalTex;
layout(binding = 4)uniform samplerBuffer uvTex;
layout(binding = 5)uniform samplerBuffer bvhTex;
layout(binding = 6)uniform sampler2D matTex;
layout(binding = 7)uniform samplerBuffer lightTex;
layout(binding = 8)uniform sampler2D envTex;
layout(binding = 9)uniform sampler2D transformTex;
layout(binding = 10)uniform sampler2DArray textureMapsArrayTex;

layout(binding = 0) uniform CommonUBO
{
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 projectionView;
    mat4 invProjection;
    mat4 invViewProjection;
    vec4 screenAndInvScreen;
    vec3 cameraPosition;
};

layout(binding = 1) uniform PathTracingUBO
{
    int maxDepth;
    int accumulateFrames;
    int topBVHIndex;
    int lightNum;
};

#define FLT_MAX 3.402823466e+38
#define PI 3.1415926
#define TWOPI 6.2831852
#define INVPI 0.31830989161
#define MIN_ROUGHNESS 0.03

struct Ray
{
    vec3 origin;
    vec3 direction;
    float Tmin;
    float TMax;
};

struct HitInfo
{
    mat4 object2WorldTransform;
    vec3 hitPosition;
    vec3 worldPosition;
    vec3 worldNormal;
    ivec3 triIndice;
    vec3 triUVW;
    vec2 barycentricUV;
    int  matID;
    float t;
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

    float baseColorTexId;
    float metallicRoughnessTexID;
    int normalmapTexID;
    float emissionmapTexID;

    float opacity;
    float alphaMode;
    float alphaCutoff;
    float padding2;
};

Material GetMatrixData(inout HitInfo hitInfo)
{
    int matIndex = 8 * hitInfo.matID;
    vec2 uv = hitInfo.barycentricUV;

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

    // if(texIDs.z >= 0)
    // {
    //     vec3 texNormal = texture(textureMapsArrayTex, vec3(uv, texIDs.z)).rgb;
    //     texNormal = normalize(texNormal * 2.0 - 1.0);
    // }

    if(texIDs.w >= 0)
    {
        data.emission = pow(texture(textureMapsArrayTex, vec3(uv, texIDs.w)).rgb, vec3(2.2));
    }

    return data;
}

struct Brdf
{
    vec3 diffuseAlbedo;
    vec3 specularF0;
    float roughness;
};

float DielectricSpecularToF0(float specular)
{
    return 0.08f * specular;
}

// F0 = mix(vec3(), albedo(baseColor), metallic);
vec3 ComputeF0(float specular, vec3 baseColor, float metalness)
{
    return mix(DielectricSpecularToF0(specular).xxx, baseColor, metalness);
}

Brdf GetBrdfData(Material material)
{
    Brdf data;
    data.diffuseAlbedo = material.baseColor * (1 - material.metallic);
    data.specularF0 = ComputeF0(material.specularTint, material.baseColor, material.metallic);
    data.roughness = material.roughness;
    return data;
}

#define DIRECTIONAL 0
#define POINT 1
#define SPOT 2
#define QUAD 3
struct Light
{
    vec4 position;
    vec4 direction;
    vec4 color;
    int bActive;
    float range;
    int type;
    float outerCosine;
    float innerCosine;
    vec3 u;
    vec3 v;
    float dis;
};

Light GetLightData(int index)
{
    int lightIndex = 6 * index;

    Light data;
    data.position = texelFetch(lightTex, lightIndex);
    data.direction = texelFetch(lightTex, lightIndex + 1);
    data.color = texelFetch(lightTex, lightIndex + 2);
    
    vec4 param4 = texelFetch(lightTex, lightIndex + 3);
    vec4 param5 = texelFetch(lightTex, lightIndex + 4);
    vec4 param6 = texelFetch(lightTex, lightIndex + 5);

    data.bActive = floatBitsToInt(param4.x);
    data.range = param4.y;
    data.type = floatBitsToInt(param4.z);
    data.outerCosine = param4.w;

    data.innerCosine = param5.x;
    data.u = param5.yzw;
    data.v = param6.xyz;

    if(data.type == QUAD)
    {
        data.direction.xyz = normalize(cross(data.u, data.v));
    }

    return data;
}

// https://github.com/chris-wyman/GettingStartedWithRTXRayTracing/blob/master/05-AmbientOcclusion/Data/Tutorial05/hlslUtils.hlsli
uint RNG_init(uint val0, uint val1, uint backoff)
{
	uint v0 = val0, v1 = val1, s0 = 0;

	for (uint n = 0; n < backoff; n++)
	{
		s0 += 0x9e3779b9;
		v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
		v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
	}
	return v0;
}

float RNG_next(inout uint s)
{
	s = (1664525u * s + 1013904223u);
	return float(s & 0x00FFFFFF) / float(0x01000000);
}

float AABBIntersect(vec3 pmin, vec3 pmax, Ray ray)
{
    vec3 invDir = 1.0f / ray.direction;

    vec3 f = (pmax - ray.origin) * invDir;
    vec3 n = (pmin - ray.origin) * invDir;

    vec3 tmax = max(f, n);
    vec3 tmin = min(f, n);

    float t1 = min(tmax.x, min(tmax.y, tmax.z));
    float t0 = max(tmin.x, max(tmin.y, tmin.z));

    return (t1 >= t0) ? (t0 > 0.0f ? t0 : t1) : -1.0f;
}

#define EPS 0.001
bool TraceRay(Ray ray, out HitInfo hitInfo)
{
    // 1. Search in TLAS and store the possible index in stack 
    // 2. Search in BLAS and if hit store the nearest t
    // 3. BackTrack from stack
    float t = FLT_MAX;
    int curMatIndex = 0;
    int stack[64];
    int ptr = 0;
    stack[ptr++] = -1;
    int index = topBVHIndex;
    bool bEnterBLAS = false;
    mat4 transform;
    Ray rayMS;
    rayMS.origin = ray.origin;
    rayMS.direction = ray.direction;
    while(index != -1)
    {
        // Each GPUNode contains 3 Vec3, and LRLeaf is the third one
        ivec3 LRLeaf = ivec3(texelFetch(bvhTex, index * 3 + 2).xyz);
        int leaf = LRLeaf.z;
        if(leaf > 0)    // BLAS's leaf
        {
            // BLAS's LRLeaf.x is the indice offset
            // BLAS's LRLeaf.y is the num of prims
            for(int i = 0; i < LRLeaf.y; i++)
            {
                ivec3 indice = ivec3(texelFetch(indiceTex, LRLeaf.x + i).xyz);

                vec3 v1 = texelFetch(vertTex, indice.x).xyz;
                vec3 v2 = texelFetch(vertTex, indice.y).xyz;
                vec3 v3 = texelFetch(vertTex, indice.z).xyz;

                // compute ray and triangles hit point using Barycentric Coord
                // [-d (B - A) (C - A)][t u v]^T = O - A
                // t = det[(O - A) (B - A) (C - A)] / det[-d (B - A) (C - A)]
                // u = det[-d (O - A) (C - A)] / det[-d (B - A) (C - A)]
                // v = det[-d (B - A) (O - A)] / det[-d (B - A) (C - A)]
                vec3 e1 = v2 - v1;
                vec3 e2 = v3 - v1;  
                // det[-d (B - A) (C - A)] = -d(cross(e1, e2)) = e1(cross(d, e2))
                vec3 h1 = cross(rayMS.direction, e2);
                float det = dot(e1, h1);

                // parallel
                // if(det > -1e-5 && det < 1e-5)
                // {
                //     continue;
                // }

                // det[(O - A) (B - A) (C - A)] = o2A(cross(e1, e2)) = e2(cross(o2A, e1))
                // det[-d (O - A) (C - A)] = -d(cross(o2A, e2)) = o2A(cross(d, e2))
                // det[-d (B - A) (O - A)] = -d(cross(e1, o2A)) = d(cross(o2A, e1))
                vec3 o2A = rayMS.origin - v1.xyz;
                vec3 h2 = cross(o2A, e1);
                vec4 tuvw;
                tuvw.x = dot(e2, h2);
                tuvw.y = dot(o2A, h1);
                tuvw.z = dot(rayMS.direction, h2);
                tuvw.xyz = tuvw.xyz / det;
                tuvw.w = 1.0f - tuvw.y - tuvw.z;

                // tuvw all components must be greater than 0
                // and hit point need the closest one
                if(all(greaterThanEqual(tuvw, vec4(0.0))) && tuvw.x < t && tuvw.x > ray.Tmin && tuvw.x <= ray.TMax)
                {
                    t = tuvw.x;
                    hitInfo.triIndice = indice;
                    hitInfo.triUVW = tuvw.wyz;
                    hitInfo.object2WorldTransform = transform;
                    hitInfo.matID = curMatIndex;
                }
            }
        }
        else if(leaf < 0)   // TLAS's leaf
        {
            // TLAS's LRLeaf.x is meshIndex
            // TLAS's LRLeaf.y is materialId
            // TLAS's LRLeaf.z is -(instanceId + 1)
            vec4 r1 = texelFetch(transformTex, ivec2((-leaf - 1) * 4 + 0, 0), 0);
            vec4 r2 = texelFetch(transformTex, ivec2((-leaf - 1) * 4 + 1, 0), 0);
            vec4 r3 = texelFetch(transformTex, ivec2((-leaf - 1) * 4 + 2, 0), 0);
            vec4 r4 = texelFetch(transformTex, ivec2((-leaf - 1) * 4 + 3, 0), 0);
            transform = mat4(r1, r2, r3, r4);

            // It is more efficient to turn rays into world space than vertices 
            mat4 invTransform = inverse(transform);
            rayMS.origin = vec3(invTransform * vec4(ray.origin, 1.0f));
            rayMS.direction = vec3(invTransform * vec4(ray.direction, 0.0f));
            
            // Enter BLAS search
            // A signal 
            stack[ptr++] = -1;
            bEnterBLAS = true;
            index = LRLeaf.x;
            curMatIndex = LRLeaf.y;
            continue;
        }
        else
        {
            float leftHit = AABBIntersect(texelFetch(bvhTex, LRLeaf.x * 3).xyz, texelFetch(bvhTex, LRLeaf.x * 3 + 1).xyz, rayMS);
            float rightHit = AABBIntersect(texelFetch(bvhTex, LRLeaf.y * 3).xyz, texelFetch(bvhTex, LRLeaf.y * 3 + 1).xyz, rayMS);
            if(leftHit > 0.0f && rightHit > 0.0f)
            {
                // notice BVH node can be overlap
                // and at this time we store the other one in stack
                if(leftHit > rightHit)
                {
                    index = LRLeaf.y;
                    stack[ptr++] = LRLeaf.x;
                }
                else
                {
                    index = LRLeaf.x;
                    stack[ptr++] = LRLeaf.y;
                }
                continue;
            }
            else if(leftHit > 0.0f)
            {
                index = LRLeaf.x;
                continue;
            }
            else if(rightHit > 0.0f)
            {
                index = LRLeaf.y;
                continue;
            }
        }
        
        // only no hit with AS or after find BLAS will enter
        index = stack[--ptr];

        if(bEnterBLAS && index == -1)
        {
            // backTrack from stack and restore the ray
            bEnterBLAS = false;
            index = stack[--ptr];
            rayMS.origin = ray.origin;
            rayMS.direction = ray.direction;
        }
    }

    if(t == FLT_MAX)
    {
        return false;
    }

    // 此处的EPS将命中点向射线方向推出一点距离，使得判定可见性时结果更准确，最终收敛更快
    hitInfo.worldPosition = ray.origin + (t - EPS) * ray.direction;
    hitInfo.t = t;

    // 将uv和法向放在循环外，仅需采样一次
    vec2 uv0 = texelFetch(uvTex, hitInfo.triIndice.x).xy;
    vec2 uv1 = texelFetch(uvTex, hitInfo.triIndice.y).xy;
    vec2 uv2 = texelFetch(uvTex, hitInfo.triIndice.z).xy;
    hitInfo.barycentricUV = uv0 * hitInfo.triUVW.x + uv1 * hitInfo.triUVW.y + uv2 * hitInfo.triUVW.z;
    
    vec3 n0 = texelFetch(normalTex, hitInfo.triIndice.x).xyz;
    vec3 n1 = texelFetch(normalTex, hitInfo.triIndice.y).xyz;
    vec3 n2 = texelFetch(normalTex, hitInfo.triIndice.z).xyz;
    vec3 localNormal = n0 * hitInfo.triUVW.x + n1 * hitInfo.triUVW.y + n2 * hitInfo.triUVW.z;
    hitInfo.worldNormal = normalize(transpose((mat3(hitInfo.object2WorldTransform))) * localNormal);

    return dot(ray.direction, hitInfo.worldNormal) < 0;
}

// 为上面TraceRay的简化版本
bool TraceShadowRay(Ray ray)
{
    int curMatIndex = 0;
    int stack[64];
    int ptr = 0;
    stack[ptr++] = -1;
    int index = topBVHIndex;
    bool bEnterBLAS = false;
    mat4 transform;
    Ray rayMS;
    rayMS.origin = ray.origin;
    rayMS.direction = ray.direction;
    while(index != -1)
    {
        ivec3 LRLeaf = ivec3(texelFetch(bvhTex, index * 3 + 2).xyz);
        int leaf = LRLeaf.z;
        if(leaf > 0)    // BLAS's leaf
        {
            for(int i = 0; i < LRLeaf.y; i++)
            {
                ivec3 indice = ivec3(texelFetch(indiceTex, LRLeaf.x + i).xyz);

                vec3 v1 = texelFetch(vertTex, indice.x).xyz;
                vec3 v2 = texelFetch(vertTex, indice.y).xyz;
                vec3 v3 = texelFetch(vertTex, indice.z).xyz;

                vec3 e1 = v2 - v1;
                vec3 e2 = v3 - v1;  

                vec3 h1 = cross(rayMS.direction, e2);
                float det = dot(e1, h1);

                // parallel
                if(det > -1e-5 && det < 1e-5)
                {
                    continue;
                }

                vec3 o2A = rayMS.origin - v1.xyz;
                vec3 h2 = cross(o2A, e1);

                vec4 tuvw;
                tuvw.x = dot(e2, h2);
                tuvw.y = dot(o2A, h1);
                tuvw.z = dot(rayMS.direction, h2);
                tuvw.xyz = tuvw.xyz / det;
                tuvw.w = 1.0f - tuvw.y - tuvw.z;

                if(all(greaterThanEqual(tuvw, vec4(0.0))) && tuvw.x >= ray.Tmin && tuvw.x <= ray.TMax)
                {
                    return true;
                }
            }
        }
        else if(leaf < 0)   // TLAS's leaf
        {
            // TLAS's LRLeaf.x is meshIndex
            // TLAS's LRLeaf.y is materialId
            // TLAS's LRLeaf.z is -(instanceId + 1)
            vec4 r1 = texelFetch(transformTex, ivec2((-leaf - 1) * 4 + 0, 0), 0);
            vec4 r2 = texelFetch(transformTex, ivec2((-leaf - 1) * 4 + 1, 0), 0);
            vec4 r3 = texelFetch(transformTex, ivec2((-leaf - 1) * 4 + 2, 0), 0);
            vec4 r4 = texelFetch(transformTex, ivec2((-leaf - 1) * 4 + 3, 0), 0);
            transform = mat4(r1, r2, r3, r4);

            // It is more efficient to turn rays into world space than vertices 
            mat4 invTransform = inverse(transform);
            rayMS.origin = vec3(invTransform * vec4(ray.origin, 1.0f));
            rayMS.direction = vec3(invTransform * vec4(ray.direction, 0.0f));
            
            // Enter BLAS search
            // A signal 
            stack[ptr++] = -1;
            bEnterBLAS = true;
            index = LRLeaf.x;
            curMatIndex = LRLeaf.y;
            continue;
        }
        else
        {
            float leftHit = AABBIntersect(texelFetch(bvhTex, LRLeaf.x * 3).xyz, texelFetch(bvhTex, LRLeaf.x * 3 + 1).xyz, rayMS);
            float rightHit = AABBIntersect(texelFetch(bvhTex, LRLeaf.y * 3).xyz, texelFetch(bvhTex, LRLeaf.y * 3 + 1).xyz, rayMS);
            if(leftHit > 0.0f && rightHit > 0.0f)
            {
                // notice BVH node can be overlap
                // and at this time we store the other one in stack
                if(leftHit > rightHit)
                {
                    index = LRLeaf.y;
                    stack[ptr++] = LRLeaf.x;
                }
                else
                {
                    index = LRLeaf.x;
                    stack[ptr++] = LRLeaf.y;
                }
                continue;
            }
            else if(leftHit > 0.0f)
            {
                index = LRLeaf.x;
                continue;
            }
            else if(rightHit > 0.0f)
            {
                index = LRLeaf.y;
                continue;
            }
        }
        
        // only no hit with AS or after find BLAS will enter
        index = stack[--ptr];

        if(bEnterBLAS && index == -1)
        {
            // backTrack from stack and restore the ray
            bEnterBLAS = false;
            index = stack[--ptr];
            rayMS.origin = ray.origin;
            rayMS.direction = ray.direction;
        }
    }

    return false;
}

float TraceShadow(vec3 worldPos, inout Light lightInfo, vec2 randVec2)
{
    Ray ray2Light;
    ray2Light.origin = worldPos;
    ray2Light.Tmin = 0;
    bool bAnyHit = false;
    if(lightInfo.type == DIRECTIONAL)
    {
        ray2Light.direction = -lightInfo.direction.xyz;
        ray2Light.TMax = FLT_MAX;
        bAnyHit = TraceShadowRay(ray2Light);
    }
    else if(lightInfo.type == POINT)
    {
        ray2Light.TMax = length(lightInfo.position.xyz - worldPos);
        ray2Light.direction = (lightInfo.position.xyz - worldPos) / ray2Light.TMax;

        lightInfo.direction = vec4(-ray2Light.direction, 1.0f);
        lightInfo.dis = ray2Light.TMax;

        bAnyHit = TraceShadowRay(ray2Light);
    }
    else if(lightInfo.type == SPOT)
    {

    }
    else if(lightInfo.type == QUAD)
    {
        // 随机在矩形光源上采样一点作为光源
        vec3 xAxis = lightInfo.u - lightInfo.position.xyz, yAxis = lightInfo.v - lightInfo.position.xyz;
        vec3 lightNormal = cross(xAxis, yAxis);
        vec3 lightPos = lightInfo.position.xyz + randVec2.x * xAxis + randVec2.y * yAxis;

        ray2Light.TMax = length(lightPos - worldPos);
        ray2Light.direction = (lightPos - worldPos) / ray2Light.TMax;

        lightInfo.direction = vec4(-ray2Light.direction, 1.0f);
        lightInfo.dis = ray2Light.TMax;

        bAnyHit = dot(lightNormal, ray2Light.direction) >= 0 ? true : TraceShadowRay(ray2Light);
    }
    return bAnyHit ? 0.0f : 1.0f;
}

// from RTXGI Resampled Importance Sampling
#define RIS_CANDIDATES_LIGHTS 8
bool SampleLightRIS(inout uint rng, vec3 position, vec3 normal, out int lightIndex, inout float sampleWeight)
{
    float totalWeights = 0.0f;
    float samplePDF = 0.0f;

    if(lightNum == 1)
    {
        return true;
    }

    int candidateMax = min(lightNum, RIS_CANDIDATES_LIGHTS);
    for(int i = 0; i < candidateMax; i++)
    {
        // use RNG and get a random light
        int randomLightIndex = min(int(RNG_next(rng) * lightNum), lightNum - 1);
        float candidateWeight = float(lightNum);

        Light lightInfo = GetLightData(randomLightIndex);

        float candidatePdf;
        float candidateRISWeight = candidatePdf * candidateWeight;

        totalWeights += candidateRISWeight;
        if(RNG_next(rng) < candidateRISWeight / totalWeights)
        {
            lightIndex = randomLightIndex;
            samplePDF = candidatePdf;
        }
    }

    if(totalWeights == 0.0f)
    {
        return false;
    }

    sampleWeight = (totalWeights / candidateMax) / samplePDF;
    return true;
}

// 法线分布项
float D_GGX(vec3 N, vec3 H, float a)
{
    float a2 = clamp(a * a, 0.0001f, 1.0f);
    float NdotH = clamp(dot(N, H), 0.0f, 1.0f);
    
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return a2 / (max(denom, 0.001f));
}

// 几何遮蔽项
float V_SmithGGX(vec3 N, vec3 V, vec3 L, float a)
{
    float a2 = clamp(a * a, 0.0001f, 1.0f);
    float NdotV = clamp(abs(dot(N, V)) + 1e-5, 0.0f, 1.0f);
    float NdotL = clamp(dot(N, L), 0.0f, 1.0f);

    float G_V = NdotV + sqrt((NdotV - NdotV * a2) * NdotV + a2);
    float G_L = NdotL + sqrt((NdotL - NdotL * a2) * NdotL + a2);
    return 1.0f / (G_V * G_L);
}

// Fresnel项
vec3 F_Schlick(vec3 V, vec3 H, vec3 F0)
{
    float VdotH = clamp(dot(V, H) + 1e-5, 0.0f, 1.0f);
    return F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
}

vec3 SpecularBRDF(vec3 N, vec3 V, vec3 L, vec3 specular, float roughness, out vec3 F)
{
    roughness = max(roughness, MIN_ROUGHNESS);

    float a = roughness * roughness;
    vec3 H = normalize(V + L);

    float D = D_GGX(N, H, a);
    float Vis = V_SmithGGX(N, V, L, a);
    F = F_Schlick(V, H, specular);

    // DFG
    return D * Vis * F;
}

vec3 DefaultBRDF(vec3 L, vec3 V, vec3 N, vec3 diffuse, vec3 specular, float roughness)
{
    vec3 F;
    vec3 specularBrdf = SpecularBRDF(N, V, L, specular, roughness, F);
    // F -- Specular reflection ratio, so 1 - F is the diffuse ratio
    vec3 diffuseBrdf = diffuse * INVPI * (1.0 - F); 

    return diffuseBrdf + specularBrdf;
}

float luminance(vec3 color) 
{
    return dot(color, vec3(0.299, 0.587, 0.114));
}

// from RTXGI 
float GetSpecularBrdfProbability(vec3 diffuseAlbedo, vec3 specularF0)
{
    float lumDiffuse = luminance(diffuseAlbedo);
    float lumSpecularF0 = luminance(specularF0);

    // float specular, diffuse;
    float probability = (lumSpecularF0 / max(0.0001f, (lumSpecularF0 + lumDiffuse)));
    return clamp(probability, 0.1f, 0.9f);
}

// Utility function to get a vector perpendicular to an input vector 
// (from "Efficient Construction of Perpendicular Vectors Without Branching")
vec3 GetPerpVector(vec3 u)
{
    vec3 a = abs(u);
    uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
    uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
    uint zm = 1 ^ (xm | ym);
    return cross(u, vec3(xm, ym, zm));
}

// 生成一个半球上符合余弦分布的样本（cos-weighted 重要性采样）
vec3 GetCosHemisphereSample(vec3 hitNormal, vec2 randVec2)
{
    // 由hit点的法向生成切线和副切线方向
    vec3 bitangent = GetPerpVector(hitNormal);
    vec3 tangent = cross(bitangent, hitNormal);
    // 计算随机样本的球极坐标
    // 原推导：theta = cos^-1(sqrt(1 - randVec2.x))，此处为余弦分布的另一种推导，采样在单位球上
    float r = sqrt(randVec2.x);
    float phi = TWOPI * randVec2.y;
    // 还原回笛卡尔坐标系
    return tangent * r * cos(phi) + bitangent * r * sin(phi) + hitNormal * sqrt(1 - randVec2.x);
}

vec3 GetImportanceGGXSample(vec3 hitNormal, float roughness, vec2 randVec2)
{
    float a2 = roughness * roughness;
    a2 = a2 * a2;
    float cosThetaH = sqrt(max(0.0f, (1.0f - randVec2.x) / ((a2 - 1.0f) * randVec2.x + 1)));
    float sinThetaH = sqrt(max(0.0f, 1 - cosThetaH * cosThetaH));
    float phiH = randVec2.y * TWOPI;

    vec3 H = vec3(cos(phiH) * sinThetaH, sin(phiH) * sinThetaH, cosThetaH);

    // GGX生成的半角向量H是基于切线空间的，因而需要转至世界空间
    vec3 bitangent = GetPerpVector(hitNormal);
    vec3 tangent = cross(bitangent, hitNormal);
    vec3 sampleVec = bitangent * H.x + tangent * H.y + hitNormal * H.z;

    return normalize(sampleVec);
}

vec3 OffsetRay(vec3 p, vec3 n) 
{
    // 常量定义
    const float origin = 1.0 / 32.0;
    const float floatScale = 1.0 / 65536.0;
    const float intScale = 256.0;

    // 计算法线的整数部分
    ivec3 of_i = ivec3(intScale * n.x, intScale * n.y, intScale * n.z);

    // 根据法线调整位置
    vec3 p_i = vec3(
        intBitsToFloat(floatBitsToInt(p.x) + (p.x < 0.0 ? -of_i.x : of_i.x)),
        intBitsToFloat(floatBitsToInt(p.y) + (p.y < 0.0 ? -of_i.y : of_i.y)),
        intBitsToFloat(floatBitsToInt(p.z) + (p.z < 0.0 ? -of_i.z : of_i.z))
    );

    // 判断是否需要偏移并返回最终结果
    return vec3(
        abs(p.x) < origin ? p.x + floatScale * n.x : p_i.x,
        abs(p.y) < origin ? p.y + floatScale * n.y : p_i.y,
        abs(p.z) < origin ? p.z + floatScale * n.z : p_i.z
    );
}

void main()
{
    // 初始化随机数
    uint rng = RNG_init(uint(gl_FragCoord.x + gl_FragCoord.y * screenAndInvScreen.x), accumulateFrames, 16);
    vec2 offset = vec2(RNG_next(rng), RNG_next(rng));
    vec2 samplePixel = gl_FragCoord.xy + mix(vec2(-0.5f), vec2(0.5f), offset);
    vec2 ndcPixel = 2.0f * (samplePixel / screenAndInvScreen.xy) - 1.0f;

    // Inverse深度缓冲，此处1代表近平面，0代表原平面
    vec4 start = invViewProjection * vec4(ndcPixel, 1.0f, 1.0f);
    start.xyz /= start.w;
    vec4 end = invViewProjection * vec4(ndcPixel, 0.0f, 1.0f);
    end.xyz /= end.w;

    Ray ray;
    ray.origin = start.xyz;
    ray.direction = normalize(end.xyz - start.xyz);
    ray.Tmin = 0;
    ray.TMax = FLT_MAX;

    vec3 radiance = vec3(0);
    vec3 throughput = vec3(1.0f);
    float pdf = 1.0f;

    for(int i = 0; true; i++)
    {
        HitInfo hitInfo;
        if(TraceRay(ray, hitInfo))
        {
            Material mat = GetMatrixData(hitInfo);
            Brdf brdf = GetBrdfData(mat);
            vec3 wo = normalize(cameraPosition - hitInfo.worldPosition);

            if(length(mat.emission) != 0) // 命中光源，不递归计算
            {
                radiance += (mat.emission) * throughput / pdf;
                break;
            }
            else // 直接光
            {
                int lightIndex = 0;
                float lightWeight = 1.0f;
                // 避免对每个光源都进行追踪，因而选择最重要的一个，方法为重要性重采样RIS
                if(SampleLightRIS(rng, hitInfo.worldPosition, hitInfo.worldNormal, lightIndex, lightWeight))
                {
                    Light lightInfo = GetLightData(lightIndex);
                    // 可见性，路径追踪中为0或1
                    float visibility = TraceShadow(hitInfo.worldPosition, lightInfo, vec2(RNG_next(rng), RNG_next(rng)));
                    vec3 wi = normalize(-lightInfo.direction.xyz);
                    float NdotL = clamp(dot(hitInfo.worldNormal, wi), 0.0f, 1.0f); 
                    float attenuation = lightInfo.type == DIRECTIONAL ? 1.0f : (1.0f / (1 + lightInfo.dis * lightInfo.dis));
                    vec3 directLighting = DefaultBRDF(wi, wo, hitInfo.worldNormal, brdf.diffuseAlbedo, brdf.specularF0, brdf.roughness) * visibility * lightInfo.color.rgb * attenuation * NdotL;
                    // 计算直接光对原着色点的贡献
                    radiance += lightWeight * (directLighting) * throughput / pdf;
                }
            }

            // 调试输出
            // if(i == maxDepth - 1)
            // if(i == 2)
            // {
            //     // color = vec4(mat.emission, 1.0);
            //     // accum = vec4(hitInfo.t, 0, 0, 1.0f);
            //     radiance = mat.emission;
            //     break;
            // }

            // 无须计算当前命中点生成的光线
            if(i == maxDepth - 1)
            {
                break;
            }

            // 下一次射线入射方向 <=> 本次出射方向
            vec3 wi;
            // 此处由Brdf随机计算此次光线的反射类型，也可以直接由材质确定反射类型
            float probSpecular = GetSpecularBrdfProbability(brdf.diffuseAlbedo, brdf.specularF0); 
            bool bDiffuse = RNG_next(rng) > probSpecular;
            if(bDiffuse)
            {
                vec3 diffuseBrdf = brdf.diffuseAlbedo * INVPI;
                // 漫反射的出射方向由cos-weighted采样得到
                wi = GetCosHemisphereSample(hitInfo.worldNormal, vec2(RNG_next(rng), RNG_next(rng)));

                float NdotL = clamp(dot(hitInfo.worldNormal, wi), 0.0f, 1.0f);
                throughput *= diffuseBrdf * NdotL;
                pdf *= (NdotL / PI) * (1 - probSpecular);
            }
            else
            {
                // 高光的出射方向使用GGX重要性采样获取
                vec3 H = GetImportanceGGXSample(hitInfo.worldNormal, brdf.roughness, vec2(RNG_next(rng), RNG_next(rng)));
                wi = reflect(-wo, H);
                float roughness = max(brdf.roughness, 0.065);

                vec3 F;
                vec3 specularBrdf = SpecularBRDF(hitInfo.worldNormal, wo, wi, brdf.specularF0, roughness, F);
                float NdotL = clamp(dot(hitInfo.worldNormal, wi), 0.0f, 1.0f);

                throughput *= specularBrdf * NdotL;

                float a = roughness * roughness;
                float D = D_GGX(hitInfo.worldNormal, H, a);
                float NdotH = clamp(dot(hitInfo.worldNormal, H), 0.0f, 1.0f);
                float LdotH = clamp(dot(wi, H), 0.0f, 1.0f);
                float NdotV = clamp(dot(hitInfo.worldNormal, wo), 0.0f, 1.0f);
                float samplePDF = D * NdotH / (4 * LdotH);
                pdf *= samplePDF * probSpecular;
            }

            // ray.origin = hitInfo.worldPosition;
            ray.origin = OffsetRay(hitInfo.worldPosition, hitInfo.worldNormal);
            ray.direction = wi;
            ray.Tmin = 1e-2;
            ray.TMax = FLT_MAX;
        }
        else
        {
            //radiance += texture(envTex, uv).rgb * throughput / pdf;
            // 暂未读取环境贴图，赋默认值
            // radiance += vec3(0.310f, 0.404f, 0.541f) * throughput / pdf;
            break;
        }
    }

    if (any(isnan(radiance)) || any(isinf(radiance)))
    {
        radiance = vec3(1.0f, 0, 0);
    }

    vec3 accumColor = texelFetch(accumTex, ivec2(gl_FragCoord.xy), 0).rgb;
    vec3 outputColor = radiance;
    // if(accumulateFrames > 1)
    // {
    //     outputColor = mix(outputColor, accumColor / accumulateFrames, 0.99);
    // }
    // color = vec4(outputColor, 1.0f);

    color = vec4((outputColor + accumColor) / accumulateFrames, 1.0f);
    accum = vec4(radiance + accumColor, 1.0f);
}