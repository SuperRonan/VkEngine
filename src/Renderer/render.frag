#version 460

#include "common.glsl"

#include <ShaderLib:/Rendering/Materials/PBMaterial.glsl>

#define BIND_SCENE 1
#define LIGHTS_ACCESS readonly
#include <ShaderLib:/Rendering/Scene/Scene.glsl>

layout(location = 0) in vec3 v_w_position;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec3 v_w_normal;

layout(location = 0) out vec4 o_color;

vec3 shade()
{
	vec3 res = 0..xxx;
	const PBMaterialProperties props = material_props.props;
	const vec2 uv = v_uv;
	const vec3 position = v_w_position; 
	const vec3 normal = normalize(v_w_normal);
	const vec3 albedo = getBoundMaterialAlbedo(v_uv);

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

void main()
{


	o_color = vec4(shade(), 1);
}