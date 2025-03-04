#pragma once

#include "Random.slang"

vec3 HSVToRGB(vec3 hsv)
{
	const float h = hsv.x * 6;	
	vec3 res;
	const vec3 N = {5, 3, 1};
	for(uint i = 0; i < 3; ++i)
	{
		const float n = N[i];
		const float k = fmod((n + h), 6);
		const float f = hsv.z - hsv.z * hsv.y * max(0, min(1, min(k, 4 - k)));
		res[i] = f;
	}

	return res;
}


vec3 RGBFromIndex(uint id)
{
	RNG_t rng = RNG_t(id);
	float hue = rng.generate<float>();
	vec3 res = HSVToRGB(vec3(hue, 1, 0.7));
	return res;
}

