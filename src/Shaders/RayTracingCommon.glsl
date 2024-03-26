#include "common.glsl"

#extension GL_EXT_ray_flags_primitive_culling : enable
#extension GL_EXT_ray_query : enable
#extension GL_EXT_ray_tracing : enable

#if GL_EXT_ray_query || GL_EXT_ray_tracing
#define CAN_BIND_TLAS 1
#else
#define CAN_BIND_TLAS 0
#endif

#define TLAS_t accelerationStructureEXT
#define RayQuery_t rayQueryEXT