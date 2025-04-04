#pragma once
#include <vec4.hpp>
using namespace glm;

enum LightType
{
    DIRECTIONAL_LIGHT,
    POINT_LIGHT,
    SPOT_LIGHT,
};

struct Light
{
    vec4 position;
    vec4 direction;
    vec4 color;

    int active;
    float range;
    LightType type;
    float outerCosine;

    float innerCosine;
    int     volumetric;
	float   volumetricStrength;
	int     useCascades;
    
	int     shadowTextureIndex;
	int     shadowMatrixIndex;
    int     shadowMaskIndex;
    int     padd;
};
