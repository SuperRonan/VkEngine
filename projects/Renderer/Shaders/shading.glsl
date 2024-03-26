#pragma once

#include "common.glsl"
#define BIND_SCENE 1
#define LIGHTS_ACCESS readonly
#include <ShaderLib:/Rendering/Scene/Scene.glsl>

#include <ShaderLib:/Rendering/CubeMap.glsl>

layout(SHADER_DESCRIPTOR_BINDING + 6) uniform sampler LightDepthSampler;

struct LightSample
{
	vec3 Le;
	vec3 direction_to_light;
};

LightSample getLightSample(uint light_id, vec3 position, vec3 normal, bool back_face_shading)
{
	LightSample res;
	const Light light = lights_buffer.lights[light_id];
	const uint light_type = light.flags & LIGHT_TYPE_MASK;
	if(light_type == LIGHT_TYPE_POINT)
	{
		const vec3 to_light = light.position - position;
		const float dist2 = dot(to_light, to_light);
		const vec3 dir_to_light = normalize(to_light);
		res.Le = light.emission / dist2;
		res.direction_to_light = dir_to_light;

		const float cos_theta = dot(normal, res.direction_to_light);
		const bool facing_light = cos_theta > 0.0f;
		const bool query_shadow_map = ((light.flags & LIGHT_ENABLE_SHADOW_MAP_BIT) != 0) && length2(res.Le) > 0 && (facing_light || back_face_shading) && (light.textures.x != uint(-1));
		if(query_shadow_map)
		{
			float ref_depth = cubeMapDepth(position, light.position, POINT_LIGHT_DEFAULT_Z_NEAR);
			int offset = 32;
			//offset = max(offset, int(cos_theta * 2));
			//ref_depth = floatOffset(ref_depth, -offset);
			ref_depth = ref_depth * 0.999;
			float texture_depth = texture(samplerCubeShadow(LightsDepthCube[light.textures.x], LightDepthSampler), vec4(-to_light, ref_depth));
			res.Le *= texture_depth;

			// Color depending on the face of the cube map
			// {
			// 	uint cube_id = findCubeDirectionId(-to_light);
			// 	vec3 color = 0..xxx;
			// 	color[cube_id / 2] = 1;
			// 	if(cube_id % 2 == 1)
			// 	{
			// 		color[cube_id / 2] *= 0.5;
			// 	}
			// 	res.Le *= color;
			// }
		}
	}
	else if(light_type == LIGHT_TYPE_DIRECTIONAL)
	{
		const vec3 dir_to_light = light.position;
		const float cos_theta = max(0.0f, dot(normal, dir_to_light));
		res.Le = light.emission;
		res.direction_to_light = dir_to_light;
	}
	else if (light_type == LIGHT_TYPE_SPOT)
	{
		const mat4 light_matrix = light.matrix;
		vec4 position_light_h = (light_matrix * vec4(position, 1));
		vec3 position_light = position_light_h.xyz / position_light_h.w;
		const vec3 to_light = light.position - position;
		res.direction_to_light = to_light;
		res.Le = 0..xxx;
		if(position_light_h.z > 0)
		{
			const float dist2 = dot(to_light, to_light);
			const vec3 dir_to_light = normalize(to_light);
			const float cos_theta = max(0.0f, dot(normal, dir_to_light));
			const vec2 clip_uv_in_light = position_light.xy;
			if(clip_uv_in_light == clamp(clip_uv_in_light, -1.0.xx, 1.0.xx))
			{
				res.Le = light.emission / (dist2);// * position_light.z;
				
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

				const float cos_theta = dot(normal, res.direction_to_light);
				const bool facing_light = cos_theta > 0.0f;
				const bool query_shadow_map = ((light.flags & LIGHT_ENABLE_SHADOW_MAP_BIT) != 0) && length2(res.Le) > 0 && (facing_light || back_face_shading) && (light.textures.x != uint(-1));
				if(query_shadow_map)
				{
					vec2 light_tex_uv =  clipSpaceToUV(clip_uv_in_light);
					float ref_depth = position_light.z;
					int offset = 4;
					//offset = max(offset, int(cos_theta * 2));
					ref_depth = floatOffset(ref_depth, -offset);
					float texture_depth = texture(sampler2DShadow(LightsDepth2D[light.textures.x], LightDepthSampler), vec3(light_tex_uv, ref_depth));
					res.Le *= texture_depth;
				}
			}
		}
		else
		{
			res.Le = 0..xxx;
		}
	}
	return res;
}

vec3 shade(vec3 albedo, vec3 position, vec3 normal)
{
	vec3 res = 0..xxx;

	vec3 diffuse = 0..xxx;

	for(uint l = 0; l < scene_ubo.num_lights; ++l)
	{
		const LightSample light_sample = 	getLightSample(l, position, normal, false);
		const float cos_theta = max(0.0f, dot(normal, light_sample.direction_to_light));
		diffuse += cos_theta * light_sample.Le; 
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

	{
#if SHADER_RAY_QUERY_AVAILABLE
		RayQuery_t rq;
		rayQueryInitializeEXT(rq, SceneTLAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, position, EPSILON * 8 * 8, normal, 10000);
		while(rayQueryProceedEXT(rq))
		{
			if (rayQueryGetIntersectionTypeEXT(rq, false) == gl_RayQueryCandidateIntersectionTriangleEXT)
			{
				rayQueryConfirmIntersectionEXT(rq);
			}
		}

		if(rayQueryGetIntersectionTypeEXT(rq, true) == gl_RayQueryCommittedIntersectionNoneEXT)
		{
			res *= vec3(1, 0, 0);
		}
		else
		{
			res *= vec3(0, 1, 0);
		}
#endif
	}

	return res;
}