#pragma once

#include <ShaderLib:/common.glsl>
#include <ShaderLib:/Maths/transforms.glsl>

#define NORMAL vec3(0, 1, 0)

float EvaluateSphericalFunction(uint index, vec3 direction, vec3 wo)
{
	float res = 0;
	res = 1.0f;
	return res;
}


vec2 GetThreadUV(uvec2 thread_id, uvec2 dims)
{
	return vec2(thread_id) / vec2(dims - uvec2(1));
}

// x: azimuth
// y: inclination
uvec2 ModulateMaxPlusOne(uvec2 id, uvec2 resolution)
{
	uvec2 res = id;
	res.x = id.x % resolution.x;
	if(id.y == resolution.y)
	{
		res.y = resolution.y - 1;
		res.x += resolution.x / 2;
	}
	return res;
}