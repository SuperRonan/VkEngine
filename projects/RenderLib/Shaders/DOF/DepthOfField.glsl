#pragma once

#include <ShaderLib:/common.glsl>

#define DEPTH_OF_FIELD_METHOD_NONE 0
#define DEPTH_OF_FIELD_METHOD_EXPLICIT 1

struct DepthOfFieldParams
{
	float radius;
	uint method;
	vec2 center_uv;
};

vec4 computeDOF(texture2D source_color, sampler source_sampler, const in DepthOfFieldParams params)
{
	vec4 res = vec4(0);

	ivec2 tex_size = textureSize(sampler2D(source_color, source_sampler), 0).xy;
	vec2 tex_size_f = vec2(tex_size);
	vec2 ratio = vec2(tex_size_f.y / tex_size_f.x, 1);

	const uint N = 8;
	for(uint i = 0; i < N; ++i)
	{
		for(uint j = 0; j < N; ++j)
		{
			vec2 uv_in_lens = UVToClipSpace(pixelToUV(uvec2(i, j), N.xx));
			float importance = 1;
			if(length2(uv_in_lens) > 1)
			{
				importance = 0;
			}
			if(importance > 0)
			{
				vec2 uv = params.center_uv + uv_in_lens * params.radius * ratio;
				res += importance * textureLod(sampler2D(source_color, source_sampler), uv, 0);
			}
		}
	}
	res /= sqr(N);

	return res;
}

