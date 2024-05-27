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
		const vec3 dir_to_light = light.direction;
		res.Le = light.emission;
		res.direction_to_light = dir_to_light;
		res.dist_to_light = POSITIVE_INF_f;
	}
	else if (light_type == LIGHT_TYPE_SPOT)
	{
		const mat4 light_matrix = getSpotLightMatrixWorldToProj(light);
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

float applyShadowBias2(float depth, const in Light light, vec3 geometry_normal)
{
	return depth;
}

// cos_theta: abs(dot(light_forward_dir, geometry_normal))
float applyShadowBias(float depth, uint light_flags, uint bias_data, float cos_theta)
{
	const uint bias_mode = light_flags & LIGHT_SHADOW_MAP_BIAS_MODE_MASK;
	const bool include_theta = (light_flags & LIGHT_SHADOW_MAP_BIAS_INCLUDE_THETA_BIT) != 0;
	float res = depth;

	if(bias_mode == LIGHT_SHADOW_MAP_BIAS_MODE_OFFSET)
	{
		const int offset = int(bias_data);
		res = floatOffset(depth, -offset);
	}
	else if(bias_mode == LIGHT_SHADOW_MAP_BIAS_MODE_FLOAT_MULT)
	{
		float m = uintBitsToFloat(bias_data);
		if(include_theta)
		{
			m = lerp(1, m*m, cos_theta);
		}
		res = depth * m;
	}
	else if(bias_mode == LIGHT_SHADOW_MAP_BIAS_MODE_FLOAT_ADD)
	{
		const float b = uintBitsToFloat(bias_data);
		res = depth + b;
	}

	return res;
}

// cos_theta: abs(dot(light_forward_dir, geometry_normal))
float computeShadow(vec3 position, vec3 geometry_normal, const in LightSample light_sample)
{
	float res = 1;
#if SHADING_SHADOW_METHOD == SHADING_SHADOW_MAP
	const Light light = lights_buffer.lights[light_sample.light_id];
	const uint light_type = light.flags & LIGHT_TYPE_MASK;
	bool query_shadow_map = ((light.flags & LIGHT_ENABLE_SHADOW_MAP_BIT) != 0) && (light.textures.x != uint(-1));
	if(query_shadow_map)
	{
		if(light_type == LIGHT_TYPE_POINT)
		{
			const vec4 ref_direction_depth = cubeMapDirectionAndDepth(position, light.position, getLightZNear(light));
			float ref_depth = ref_direction_depth.w;
			const vec3 light_main_direction = ref_direction_depth.xyz;
			// Could also compute the sin_theta with the norm of the cross produc
			const float cos_theta = adot(light_main_direction, geometry_normal);
			const float sin_theta = sqrt(1 - sqr(cos_theta));
			const ivec2 tex_size = textureSize(samplerCubeShadow(LightsDepthCube[light.textures.x], LightDepthSampler), 0);
			
			const float shadow_texel_size = sin_theta * ref_depth / tex_size.x;
			vec2 lerp_range = vec2(4096, 128);
			int offset = int(lerp(lerp_range.x, lerp_range.y,shadow_texel_size ));
			offset = int(1024 * (1 + sin_theta));
			ref_depth = floatOffset(ref_depth, -offset);

			//ref_depth = applyShadowBias(ref_depth, light.flags, light.shadow_bias_data, cos_theta);

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
			const float ref_depth = light_sample.ref_depth;
			const vec3 light_main_direction = light.direction;
			const float cos_theta = adot(light_main_direction, geometry_normal);
			const float sin_theta = sqrt(1 - sqr(cos_theta));
			const ivec2 tex_res = textureSize(sampler2DShadow(LightsDepth2D[light.textures.x], LightDepthSampler), 0);
			const float tex_size = float(tex_res.x);
			const float shadow_texel_size = sin_theta * (ref_depth) / (log2(tex_size));
			vec2 lerp_range = vec2(16, 128 * 8);
			int offset = int(lerp(lerp_range.x, lerp_range.y, shadow_texel_size));
			offset = int(64 * (1 + sin_theta));
			const float biased_ref_depth = floatOffset(ref_depth, -offset);

			//ref_depth = applyShadowBias(ref_depth, light.flags, light.shadow_bias_data, cos_theta);
			const float texture_depth_test = texture(sampler2DShadow(LightsDepth2D[light.textures.x], LightDepthSampler), vec3(light_tex_uv, biased_ref_depth));
			res = texture_depth_test;
			//const float texture_depth = texture(sampler2D(LightsDepth2D[light.textures.x], LightDepthSampler), vec2(light_tex_uv)).x;
			//res = (biased_ref_depth <= texture_depth) ? 1 : 0;
		}
	}
#elif SHADING_SHADOW_METHOD == SHADING_SHADOW_RAY_TRACE
	Ray ray = Ray(position, light_sample.direction_to_light);
	const float t_min = rayTMin(ray, geometry_normal);
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

	const vec3 geometry_normal = normal;

	vec3 diffuse = 0..xxx;

	for(uint l = 0; l < scene_ubo.num_lights; ++l)
	{
		const LightSample light_sample = getLightSample(l, position, normal, BSDF_REFLECTION_BIT);
		if(light_sample.pdf > 0)
		{
			const float cos_theta = dot(normal, light_sample.direction_to_light);
			const float abs_cos_theta = abs(cos_theta);
			const float bsdf_rho = cos_theta > 0 ? 1 : 0;

			float shadow = 1;
			if(length2(light_sample.Le * bsdf_rho) > 0)
			{
				// TODO geometry normal
				shadow = computeShadow(position, geometry_normal, light_sample);
			}
			diffuse += (bsdf_rho * abs_cos_theta * light_sample.Le * shadow) / light_sample.pdf; 
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