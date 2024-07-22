#pragma once

#include <ShaderLib:/common.glsl>
#include <ShaderLib:/Maths/transforms.glsl>

#define NORMAL vec3(0, 1, 0)

float EvaluateSphericalFunction(uint index, vec3 wo, vec3 wi)
{
	const vec3 n = NORMAL;
	const vec3 halfway = safeNormalize(wo + wi);
	const vec3 reflected = reflect(-wo, n);

	const float cos_theta_i = dot(n, wi);
	const float abs_cos_theta_i = abs(cos_theta_i);

	float res = 0;

	if(index == 0)
	{
		if(cos_theta_i > 0)
		{
			res = oo_PI;
		}
	}
	else if (index == 1)
	{
		if(cos_theta_i > 0)
		{
			res = pow(max(dot(reflected, wi), 0), 10);
		}
	}

	res *= abs_cos_theta_i;
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