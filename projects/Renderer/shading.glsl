#pragma once

#include "common.glsl"
#define BIND_SCENE 1
#define LIGHTS_ACCESS readonly
#include <ShaderLib:/Rendering/Scene/Scene.glsl>

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
				}

				diffuse += contrib;
			}
		}
	}

	res += diffuse;

	return res;
}