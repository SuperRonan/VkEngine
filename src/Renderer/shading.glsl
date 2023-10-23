#pragma once

#include "common.glsl"
#define BIND_SCENE 1
#define LIGHTS_ACCESS readonly
#include <ShaderLib:/Rendering/Scene/Scene.glsl>

vec3 shade(vec3 albedo, vec3 position, vec3 normal)
{
	vec3 res = 0..xxx;

	res += albedo * scene_ubo.ambient;

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
	}

	res += diffuse;

	return res;
}