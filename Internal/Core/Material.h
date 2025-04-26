#pragma once
#include <vec3.hpp>
using namespace glm;

// 8 * vec4
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
    float clearcoatGloss;

    float specTrans;
    float ior;
    float mediumType;
    float mediumDensity;
    
    vec3 mediumColor;
    float mediumAnisotropy;

    float baseColorTexId;
    float metallicRoughnessTexID;
    float normalmapTexID;
    float emissionmapTexID;

    float opacity;
    float alphaMode;
    float alphaCutoff;
    float padding2;

    Material()
    {
        baseColor = vec3(1.0f, 1.0f, 1.0f);
        anisotropic = 0.0f;

        emission = vec3(0.0f, 0.0f, 0.0f);
        // padding1

        metallic     = 0.0f;
        roughness    = 1.0f;
        subsurface   = 0.0f;
        specularTint = 0.0f;

        sheen          = 0.0f;
        sheenTint      = 0.0f;
        clearcoat      = 0.0f;
        clearcoatGloss = 0.0f;

        specTrans        = 0.0f;
        ior              = 1.5f;
        mediumType       = 0.0f;
        mediumDensity    = 0.0f;

        mediumColor      = vec3(1.0f, 1.0f, 1.0f);
        mediumAnisotropy = 0.0f;

        baseColorTexId         = -1.0f;
        metallicRoughnessTexID = -1.0f;
        normalmapTexID         = -1.0f;
        emissionmapTexID       = -1.0f;

        opacity     = 1.0f;
        alphaMode   = 0.0f;
        alphaCutoff = 0.0f;
        // padding2       
    }
};
