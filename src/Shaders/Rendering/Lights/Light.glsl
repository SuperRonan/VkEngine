#pragma once

#include <ShaderLib:/common.glsl>


#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_DIRECTIONAL 2
#define LIGHT_TYPE_MASK 7

struct Light
{
	vec3 position;
	uint flags;
	vec3 emission;
};
