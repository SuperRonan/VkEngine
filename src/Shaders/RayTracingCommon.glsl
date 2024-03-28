#pragma once
#include "common.glsl"

#if SHADER_RAY_QUERY_AVAILABLE || SHADER_RAY_TRACING_AVAILABLE
#define CAN_BIND_TLAS 1
#extension GL_EXT_ray_flags_primitive_culling : require
#else
#define CAN_BIND_TLAS 0
#endif

#if SHADER_RAY_QUERY_AVAILABLE
#extension GL_EXT_ray_query : require
#endif

#if SHADER_RAY_TRACING_AVAILABLE
#extension GL_EXT_ray_tracing : require
#endif

#define TLAS_t accelerationStructureEXT
#define RayQuery_t rayQueryEXT

float rayTMin(vec3 src, vec3 direction, vec3 normal)
{
	// Not perfect, but good enough
	float res = 0;
	const float d = abs(dot(direction, normal));
	for(uint i=0; i < 3; ++i)
	{
		res = max(res, abs(src[i]));
	}
	res /= (1 << 20);
	res *= 16 * (2.0 - d);
	res = max(res, EPSILON * 64);
	return res;
}