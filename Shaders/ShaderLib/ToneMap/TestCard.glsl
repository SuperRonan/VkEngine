#pragma once

#include <ShaderLib:/common.glsl>


float SampleTestCard(vec2 uv, float ratio, uint mode)
{
	float res = 0;
	res = uv.x;
	
	if(uv.y >= 0.5)
	{
		res = pow(res, 2.4);
	}
	if(mod(uv.y, 0.5) >= 0.25)
	{
		res = quantize(res, 1.0 / 16);
	}
	
	return res;
}


