#include "common.glsl"

#if SHADER_RAY_QUERY_AVAILABLE || SHADER_RAY_TRACING_AVAILABLE
#define CAN_BIND_TLAS 1
#extension GL_EXT_ray_flags_primitive_culling : warn
#else
#define CAN_BIND_TLAS 0
#endif

#if SHADER_RAY_QUERY_AVAILABLE
#extension GL_EXT_ray_query : enable
#endif
#if SHADER_RAY_TRACING_AVAILABLE
#extension GL_EXT_ray_tracing : enable
#endif

#define TLAS_t accelerationStructureEXT
#define RayQuery_t rayQueryEXT

float rayTMin(vec3 src, vec3 direction, vec3 normal)
{
    // TODO
    return EPSILON * 8;
}