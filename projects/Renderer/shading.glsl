#pragma once

#include "common.glsl"
#define BIND_SCENE 1
#define LIGHTS_ACCESS readonly
#include <ShaderLib:/Rendering/Scene/Scene.glsl>

layout(SHADER_DESCRIPTOR_BINDING + 6) uniform sampler LightDepthSampler;

vec3 shade(vec3 albedo, vec3 position, vec3 normal)
{
	vec3 res = 0..xxx;

	vec3 diffuse = 0..xxx;

	for(uint l = 0; l < scene_ubo.num_lights; ++l)
	{
		const Light light = lights_buffer.lights[l];
		const uint light_type = light.flags & LIGHT_TYPE_MASK;

		if(light_type == LIGHT_TYPE_POINT)
		{
			const vec3 to_light = light.position - position;
			const float dist2 = dot(to_light, to_light);
			const vec3 dir_to_light = normalize(to_light);
			const float cos_theta = max(0.0f, dot(normal, dir_to_light));

			diffuse += cos_theta * albedo * light.emission / dist2;
		}
		else if(light_type == LIGHT_TYPE_DIRECTIONAL)
		{
			const vec3 dir_to_light = light.position;
			const float cos_theta = max(0.0f, dot(normal, dir_to_light));
			diffuse += cos_theta * albedo * light.emission;
		}
		else if (light_type == LIGHT_TYPE_SPOT)
		{
			const mat4 light_matrix = light.matrix;
			vec4 position_light_h = (light_matrix * vec4(position, 1));
			vec3 position_light = position_light_h.xyz / position_light_h.w;
			if(position_light_h.z > 0)
			{
				const vec3 to_light = light.position - position;
				const float dist2 = dot(to_light, to_light);
				const vec3 dir_to_light = normalize(to_light);
				const float cos_theta = max(0.0f, dot(normal, dir_to_light));
				vec3 contrib = 0..xxx;
				vec2 clip_uv_in_light = position_light.xy;
				if(clip_uv_in_light == clamp(clip_uv_in_light, -1.0.xx, 1.0.xx))
				{
					contrib = cos_theta * light.emission * albedo / dist2;// * position_light.z;
					
					if((light.flags & SPOT_LIGHT_FLAG_ATTENUATION) != 0)
					{
						const float dist_to_center = length2(clip_uv_in_light);
						const float attenuation = max(1.0 - dist_to_center, 0);
						contrib *= attenuation;
					}

					if(length2(contrib) > 0)
					{
						vec2 light_tex_uv =  clipSpaceToUV(clip_uv_in_light);
						float ref_depth = position_light.z;
						// float offset
						ref_depth = intBitsToFloat(floatBitsToInt(ref_depth) - 8);
						float texture_depth = texture(sampler2DShadow(LightsDepth2D[light.textures.x], LightDepthSampler), vec3(light_tex_uv, ref_depth));
						contrib *= texture_depth;
						// contrib.xy = light_tex_uv * 0;
						// contrib.z = position_light.z;
						// contrib.z = (texture_depth - 0.999) * 1000;
					}
				}

				diffuse += contrib;
			}
		}
	}

	res += diffuse;

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