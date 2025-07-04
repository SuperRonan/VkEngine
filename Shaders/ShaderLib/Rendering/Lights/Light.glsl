#pragma once

#include <ShaderLib:/common.glsl>

#include <ShaderLib:/Maths/transforms.glsl>
#include "Definitions.h"

// sizeof(Light) == 32
// Wish we has unions in glsl
struct Light
{
	vec3 position;
	uint flags; // 4
	
	vec3 emission;
	// int or float
	uint shadow_bias_data; // 8
	
	uvec4 textures; // 12

	vec3 direction;
	float z_near; // 16

	uint32_t extra_params[16]; // 32
};


float getLightZNear(const in Light l)
{
	//return POINT_LIGHT_DEFAULT_Z_NEAR;
	return l.z_near;
}

vec3 getLightUp(const in Light l)
{
	const uint b = 0;
	vec3 res;
	for(uint i=0; i < 3; ++i)
	{
		res[i] = uintBitsToFloat(l.extra_params[b + i]);
	}
	return res;
}

float getLightTanHalfFov(const in Light l)
{
	return uintBitsToFloat(l.extra_params[3]);
}

float getLightAspect(const in Light l)
{
	return uintBitsToFloat(l.extra_params[4]);
}

mat4x3 getLightMatrixWorldToObject(const in Light l)
{
	const vec3 center = l.position;
	const vec3 front = l.direction;
	const vec3 up = getLightUp(l);
	return LookAtDir4x3AssumeOrtho(center, front, up, cross(up, front));
}

mat4 getSpotLightMatrixObjectToProj(const in Light l)
{
	const float tan_half_fov = getLightTanHalfFov(l);
	const float aspect = getLightAspect(l);
	const float z_near = l.z_near;
	mat4 res = InfinitePerspectiveProjFromTan(tan_half_fov, aspect, z_near);
	return res;
}

mat4 getSpotLightMatrixWorldToProj(const in Light l)
{
	const mat4 world_to_obj = mat4(getLightMatrixWorldToObject(l));
	const mat4 object_to_proj = getSpotLightMatrixObjectToProj(l);
	const mat4 res = object_to_proj * world_to_obj;
	return res;
}