#version 460

#include "common.glsl"
#include "render3D.glsl"

#include <ShaderLib:/Rendering/Mesh/Sphere.glsl>

layout(location = 0) in flat uvec2 v_type_id;
layout(location = 1) in Varying v_in;

layout(location = 0) out vec4 o_color;

#if RENDER_MODE == RENDER_MODE_3D
layout(SHADER_DESCRIPTOR_BINDING + 0, std430) buffer readonly restrict b_state
{
	Particule particules[];
} state;
#endif

layout(SHADER_DESCRIPTOR_BINDING + 1, std430) buffer readonly restrict ub_common_rules
{
	CommonRuleBuffer rules;
} common_rules_ubo;

layout(SHADER_DESCRIPTOR_BINDING + 2) uniform UBO
{
	Uniforms3D ubo;
};

#if RENDER_MODE == RENDER_MODE_3D

#endif

layout(depth_greater) out float gl_FragDepth;

// layout(push_constant) uniform FragPushConstant
// {
//     layout(offset = 64) float zoom;
// };


vec3 shade(vec3 wi, vec3 frag_pos, vec3 normal, vec3 pcolor)
{
	vec3 res = 0..xxx;

	res += ubo.ambient_light_intensity * pcolor;

	const float cos_theta = max(dot(normal, ubo.light_dir), 0.0f);
	res += cos_theta * ubo.light_intensity * pcolor;

#if false
	if(ubo.roughness != 1)
	{
		const float shininess = tan(HALF_PI * (1 - ubo.roughness));

		const vec3 wo = reflect(-wi, normal);
		const float refl_dot = max(dot(wo, ubo.light_dir), 0);

		res += pow(refl_dot, shininess);// * (shininess + 1) / TWO_PI;
	}
#endif

	return res;
}

void main()
{
	const uint ptype = v_type_id.x;
	const vec2 v_uv = v_in.uv;
#if RENDER_MODE == RENDER_MODE_2D_LIKE
	const float d = dot(v_uv, v_uv);
	if(d > 0.25)
	{
		discard;
		return;
	}
	else
	{
		o_color = common_rules_ubo.rules.particules_properties[ptype].color;

		if(d > 0.20)
		{
			o_color.xyz *= 0.5;
		}
	}
#elif RENDER_MODE == RENDER_MODE_3D

	const vec3 pcolor = common_rules_ubo.rules.particules_properties[ptype].color.xyz;

	// if(any(greaterThan(abs(v_uv), vec2(0.45))))
	// {
	// 	o_color = vec4(pcolor, 1);
	// 	return;
	// }
	const vec3 quad_world_pos = v_in.world_pos;
	Ray ray;
	ray.origin = ubo.camera_pos;
	ray.direction = normalize(quad_world_pos - ray.origin);
	const uint id = v_type_id.y;
	const Particule p = state.particules[id];

	Sphere sphere;
	sphere.center = p.position;
	sphere.radius = p.radius;

	const float ray_t = intersectOutside(sphere, ray);
	
	if(ray_t < 0.0f)
	{
		discard;
	}

	const vec3 sphere_point = sampleRay(ray, ray_t);
	const vec3 wi = normalize(ubo.camera_pos - sphere_point);
	const vec3 normal = (sphere_point - sphere.center) / sphere.radius;

	vec4 sphere_point_proj = ubo.world_to_proj * vec4(sphere_point, 1);
	gl_FragDepth = sphere_point_proj.z / sphere_point_proj.w;	


	o_color = vec4(shade(wi, sphere_point, normal, pcolor), 1);

	//o_color.xyz *= (normal * 0.5 + 0.5);

#endif
}