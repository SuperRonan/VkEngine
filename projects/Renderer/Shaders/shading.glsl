#pragma once

#include "common.glsl"

#define BIND_SCENE 1
#define LIGHTS_ACCESS readonly
#include <ShaderLib:/Rendering/Scene/Scene.glsl>

#include <ShaderLib:/Rendering/Ray.glsl>

#include <ShaderLib:/Rendering/CubeMap.glsl>

#include <ShaderLib:/RayTracingCommon.glsl>

layout(SHADER_DESCRIPTOR_BINDING + 6) uniform sampler LightDepthSampler;

#define SHADING_SHADOW_NONE 0
#define SHADING_SHADOW_MAP 1
#define SHADING_SHADOW_RAY_TRACE 2

#ifndef SHADING_SHADOW_METHOD
#define SHADING_SHADOW_METHOD SHADING_SHADOW_NONE
#endif
struct LightSample
{
	vec3 Le;
	vec3 direction_to_light;
	float dist_to_light;
	float pdf;
	uint light_type;
	uint light_id;
	vec2 uv;
	float ref_depth;
};

#define BSDF_REFLECTION_BIT 0x1
#define BSDF_TRANSMISSION_BIT  0x2
#define BSDF_HEMISPHERE_MASK 0x3

uint shadingFaceFlags(bool front)
{
	uint res = front ? BSDF_REFLECTION_BIT : BSDF_TRANSMISSION_BIT;
	return res;
}

LightSample getLightSample(uint light_id, vec3 position, vec3 normal, uint shading_flags)
{

#if SHADING_SHADOW_METHOD == SHADING_SHADOW_RAY_TRACE
	Ray ray;
	float light_dist = 0; // 0 means no ray
#endif

	LightSample res;
	const Light light = lights_buffer.lights[light_id];
	const uint light_type = light.flags & LIGHT_TYPE_MASK;
	res.light_type = light_type;
	res.light_id = light_id;
	res.pdf = 1;
	if(light_type == LIGHT_TYPE_POINT)
	{
		const vec3 to_light = light.position - position;
		const float dist2 = dot(to_light, to_light);
		const vec3 dir_to_light = normalize(to_light);
		res.Le = light.emission / dist2;
		res.direction_to_light = dir_to_light;
		res.dist_to_light = sqrt(dist2);
	}
	else if(light_type == LIGHT_TYPE_DIRECTIONAL)
	{
		const vec3 dir_to_light = light.position;
		res.Le = light.emission;
		res.direction_to_light = dir_to_light;
		res.dist_to_light = POSITIVE_INF_f;
	}
	else if (light_type == LIGHT_TYPE_SPOT)
	{
		const mat4 light_matrix = light.matrix;
		vec4 position_light_h = (light_matrix * vec4(position, 1));
		vec3 position_light = position_light_h.xyz / position_light_h.w;
		const vec3 to_light = light.position - position;
		res.Le = 0..xxx;
		if(position_light_h.z > 0)
		{
			const float dist2 = dot(to_light, to_light);
			const vec3 dir_to_light = normalize(to_light);
			res.direction_to_light = dir_to_light;
			res.dist_to_light = sqrt(dist2);
			const vec2 clip_uv_in_light = position_light.xy;
			if(clip_uv_in_light == clamp(clip_uv_in_light, -1.0.xx, 1.0.xx))
			{
				res.Le = light.emission / (dist2);// * position_light.z;
				res.uv = clipSpaceToUV(clip_uv_in_light);
				res.ref_depth = position_light.z;
				
				const uint attenuation_flags = light.flags & SPOT_LIGHT_FLAG_ATTENUATION_MASK;
				if((attenuation_flags) != 0)
				{
					float attenuation = 0;
					float dist_to_center;
					if(attenuation_flags == SPOT_LIGHT_FLAG_ATTENUATION_LINEAR)
					{
						dist_to_center = length(clip_uv_in_light);
					}
					else if(attenuation_flags == SPOT_LIGHT_FLAG_ATTENUATION_QUADRATIC)
					{
						dist_to_center = length2(clip_uv_in_light);
					}
					else if(attenuation_flags == SPOT_LIGHT_FLAG_ATTENUATION_ROOT)
					{
						dist_to_center = sqrt(length(clip_uv_in_light));
					}
					
					attenuation = max(1.0 - dist_to_center, 0);
					res.Le *= attenuation;
				}
			}
		}
		else
		{
			res.Le = 0..xxx;
			res.pdf = 0;
		}
	}
	return res;
}

float computeShadow(vec3 position, vec3 normal, const in LightSample light_sample)
{
	float res = 1;
#if SHADING_SHADOW_METHOD == SHADING_SHADOW_MAP
	const Light light = lights_buffer.lights[light_sample.light_id];
	const uint light_type = light.flags & LIGHT_TYPE_MASK;
	bool query_shadow_map = ((light.flags & LIGHT_ENABLE_SHADOW_MAP_BIT) != 0) && (light.textures.x != uint(-1));
	const int offset = 32;
	if(query_shadow_map)
	{
		if(light_type == LIGHT_TYPE_POINT)
		{
			float ref_depth = cubeMapDepth(position, light.position, POINT_LIGHT_DEFAULT_Z_NEAR);
			//offset = max(offset, int(cos_theta * 2));
			//ref_depth = floatOffset(ref_depth, -offset);
			ref_depth = ref_depth * 0.999;
			float texture_depth = texture(samplerCubeShadow(LightsDepthCube[light.textures.x], LightDepthSampler), vec4(-light_sample.direction_to_light, ref_depth));
			res = texture_depth;	
		}
		else if(light_type == LIGHT_TYPE_DIRECTIONAL)
		{
			// TODO
		}
		else if(light_type == LIGHT_TYPE_SPOT)
		{
			vec2 light_tex_uv = light_sample.uv;
			float ref_depth = light_sample.ref_depth;
			ref_depth = floatOffset(ref_depth, -offset);
			float texture_depth = texture(sampler2DShadow(LightsDepth2D[light.textures.x], LightDepthSampler), vec3(light_tex_uv, ref_depth));
			res = texture_depth;
		}
	}
#elif SHADING_SHADOW_METHOD == SHADING_SHADOW_RAY_TRACE
	Ray ray = Ray(position, light_sample.direction_to_light);
	const float t_min = rayTMin(ray, normal);
	float t_max = light_sample.dist_to_light;
	if(light_sample.light_type == LIGHT_TYPE_SPOT)
	{
		//t_max = 0.1;
	}
	bool v = QueryVisibilityRayOpaqueOnlyTriangles(SceneTLAS, ray, vec2(t_min, t_max));
	res = v ? 1.0f : 0.0f;
#endif
	return res;
}

// TODO geometry and shading normal
vec3 shade(vec3 albedo, vec3 position, vec3 normal)
{
	vec3 res = 0..xxx;

	vec3 diffuse = 0..xxx;

	for(uint l = 0; l < scene_ubo.num_lights; ++l)
	{
		const LightSample light_sample = getLightSample(l, position, normal, BSDF_REFLECTION_BIT);
		if(light_sample.pdf > 0)
		{
			const float cos_theta = max(0.0f, dot(normal, light_sample.direction_to_light));
			const float bsdf_rho = 1;

			float shadow = 1;
			if(length2(light_sample.Le * bsdf_rho) > 0)
			{
				// TODO geometry normal
				shadow = computeShadow(position, normal, light_sample);
			}

			diffuse += (bsdf_rho * cos_theta * light_sample.Le * shadow) / light_sample.pdf; 
		}
	}

	res += diffuse * albedo;

	// {
	// 	vec2 uv = (vec2(gl_GlobalInvocationID.xy) + 0.5) / vec2(2731, 1500);
	// 	//float texture_depth = textureLod(sampler2DShadow(LightsDepth2D[0], LightDepthSampler), vec3(uv, 0.99995), 0).x;
	// 	float texture_depth = textureLod(sampler2D(LightsDepth2D[0], LightDepthSampler), vec2(uv), 0).x;
	// 	float range = 1e-4;
	// 	texture_depth = (texture_depth - (1.0 - range)) / range;
	// 	res = texture_depth.xxx;
	// }

	return res;
}