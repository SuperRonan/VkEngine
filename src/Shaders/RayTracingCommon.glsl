#pragma once
#include "common.glsl"

#include "Rendering/Ray.glsl"

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

// TODO https://developer.nvidia.com/blog/solving-self-intersection-artifacts-in-directx-raytracing/

float rayTMin(vec3 src)
{
	// Not perfect, but good enough
	float res = 0;
	for(uint i=0; i < 3; ++i)
	{
		res = max(res, abs(src[i]));
	}
	res /= (1 << 20);
	res = max(res, EPSILON * 64);
	return res;
}

float rayTMin(Ray ray, vec3 normal)
{
	// Not perfect, but good enough
	float res = rayTMin(ray.origin);
	const float d = abs(dot(ray.direction, normal));
	res *= 16 * (2.0 - d);
	res = max(res, EPSILON * 64);
	return res;
}

#if SHADER_RAY_QUERY_AVAILABLE

bool QueryVisibilityRayOpaqueOnlyTriangles(TLAS_t tlas, Ray ray, vec2 range)
{
	RayQuery_t rq;
	const uint ray_flags = gl_RayFlagsSkipAABBEXT | gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT;
	const uint cull_mask = 0xFF;
	rayQueryInitializeEXT(rq, tlas, ray_flags, cull_mask, ray.origin, range.x, ray.direction, range.y);
	bool res = true;
	rayQueryProceedEXT(rq);
	res = (rayQueryGetIntersectionTypeEXT(rq, true) == gl_RayQueryCommittedIntersectionNoneEXT);
	return res;	
}

bool QueryVisibilityRayOpaqueOnlyTriangles(TLAS_t tlas, Ray ray, float t_max)
{
	const float t_min = rayTMin(ray.origin);
	return QueryVisibilityRayOpaqueOnlyTriangles(tlas, ray, vec2(t_min, t_max));
}

bool QueryVisibilityRayOpaqueOnlyTriangles(TLAS_t tlas, vec3 src, vec3 dst)
{
	const float dist = distance(src, dst);
	const vec3 dir = (dst - src) / dist;
	const float t_max = floatOffset(dist, -128);
	return QueryVisibilityRayOpaqueOnlyTriangles(tlas, Ray(src, dir), t_max);
}

#endif