#version 450

#define BSDF_IMAGE_ACCESS writeonly

#include "Bindings.glsl"

#ifndef LOCAL_SIZE_X
#define LOCAL_SIZE_X 128
#endif

#ifndef LOCAL_SIZE_Y
#define LOCAL_SIZE_Y 1
#endif

layout(local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y) in;

void main()
{
	const uint num_functions = gl_NumWorkGroups.z;
	const uvec2 num_wgs = gl_NumWorkGroups.xy;
	const uvec2 num_threads = num_wgs * uvec2(LOCAL_SIZE_X, LOCAL_SIZE_Y);
	const uvec2 lid = gl_LocalInvocationID.xy;
	const uint local_linear_thread_id = lid.y * LOCAL_SIZE_X + lid.x;
	const uvec2 wid = gl_WorkGroupID.xy;
	const uvec3 gid3 = gl_GlobalInvocationID;
	const uint function_index = gid3.z;
	const uvec2 gid = gid3.xy;

	const vec2 thread_uv = GetThreadUV(gid, num_threads);
	const vec2 theta_phi = thread_uv * vec2(M_PI, TWO_PI);
	const vec3 wi = SphericalToCartesian(theta_phi);
	const vec3 wo = ubo.direction;
	float intensity = EvaluateSphericalFunction(function_index, wo, wi);

	imageStore(bsdf_image, ivec3(gid3), intensity.xxxx);
}