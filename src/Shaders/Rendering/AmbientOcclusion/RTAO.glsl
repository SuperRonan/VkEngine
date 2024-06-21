#version 460

#define I_WANT_TO_DEBUG 0
#include <ShaderLib:/Debug/DebugBuffers.glsl>

#define BIND_SCENE 1
#include <ShaderLib:/Rendering/Scene/scene.glsl>

#include "AO_Common.glsl"

#include <ShaderLib:/common.glsl>
#include <ShaderLib:/random.glsl>
#include <ShaderLib:/Maths/transforms.glsl>


layout(SHADER_DESCRIPTOR_BINDING + 0, OUT_FORMAT) uniform writeonly restrict image2D ao_image;

layout(SHADER_DESCRIPTOR_BINDING + 1) uniform sampler2D in_world_position; 
layout(SHADER_DESCRIPTOR_BINDING + 2) uniform sampler2D in_world_normal; 



#ifndef AO_METHOD
#define AO_METHOD AO_METHOD_SSAO
#endif

#ifndef AO_SAMPLES
#define AO_SAMPLES 16 * 4
#endif

#if SHADER_SEMANTIC_RAYGEN || SHADER_SEMANTIC_COMPUTE

layout(push_constant) uniform PushConstant
{
	vec3 camera_position;
	uint flags;
	float radius;
	uint seed;
} _pc;


float testAOVisibility(TLAS_t tlas, Ray ray, vec2 range);

float computeRTAO(inout rng_t rng, vec3 position, vec3 normal, float radius)
{
	const mat3 world_basis = basisFromDir(normal);

	float res = 0.0f;

	for(uint i = 0; i < AO_SAMPLES; ++i)
	{
		const vec3 sampled_dir_local = randomCosineDirOnHemisphere(rng);
		const vec3 sampled_dir_w = world_basis * sampled_dir_local;
		Ray ray = Ray(position, sampled_dir_w);
		const float t_min = rayTMin(ray, normal);
		// The sample contribution cancels the pdf
		res += testAOVisibility(SceneTLAS, ray, vec2(t_min, radius));
	}
	return res / float(AO_SAMPLES);
}

void common_main(uvec3 tid)
{
	const uvec3 dims = uvec3(imageSize(ao_image), 1);
	const vec2 uv = pixelToUV(tid.xy, dims.xy);
	const vec2 in_tex_size = vec2(textureSize(in_world_position, 0));
	const vec3 position = texelFetch(in_world_position, ivec2(uv * in_tex_size), 0).xyz;
	vec3 normal = texelFetch(in_world_normal, ivec2(uv * in_tex_size), 0).xyz;
	normal = normalize(normal);

	const float world_radius = _pc.radius * 10;

	rng_t rng = hash(tid) ^ _pc.seed;

	float res = computeRTAO(rng, position, normal, world_radius);

	imageStore(ao_image, ivec2(tid.xy), res.xxxx);
}

#endif

#if SHADER_SEMANTIC_MISS

layout(location = 0) rayPayloadInEXT float payload;

void main()
{
	payload = 1.0f;
}

#endif

#if SHADER_SEMANTIC_CLOSEST_HIT

layout(location = 0) rayPayloadInEXT float payload;

void main()
{
	payload = 0.0f;
}

#endif

#if SHADER_SEMANTIC_RAYGEN

// RTAO

layout(location = 0) rayPayloadEXT float payload;

float testAOVisibility(TLAS_t tlas, Ray ray, vec2 range)
{
	const uint ray_flags = gl_RayFlagsSkipAABBEXT | gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT;
	const uint cull_mask = 0xFF;
	const uint sbt_offset = 0;
	const uint sbt_stride = 0;
	const uint miss_index = 0;
	traceRayEXT(tlas, ray_flags, cull_mask, sbt_offset, sbt_stride, miss_index, ray.origin, range.x, ray.direction, range.y, 0);
	return payload;
}

void main()
{
	const uvec3 tid = gl_LaunchIDEXT;
	const uvec3 launch_size = gl_LaunchSizeEXT;
	
	common_main(tid);
}

#endif


#if SHADER_SEMANTIC_COMPUTE

// RQAO

float testAOVisibility(TLAS_t tlas, Ray ray, vec2 range)
{
	if(QueryVisibilityRayOpaqueOnlyTriangles(tlas, ray, range))
	{
		return 1.0f;		
	}
	return 0.0f;
}

layout(local_size_x = 16, local_size_y = 16) in;

void main()
{
	const uvec3 tid = gl_GlobalInvocationID;
	const uvec3 dims = uvec3(imageSize(ao_image), 1);
	if(all(lessThan(tid, dims)))
	{
		common_main(tid);
	}
}

#endif