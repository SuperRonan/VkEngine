
#version 460 core

#include "common.glsl"

#line 7

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

layout(location = 0) in ivec2 vertex_id[1];

layout(location = 0) out vec3 world_position;
layout(location = 1) out vec3 world_normal;
layout(location = 2) out vec2 o_uv;

layout(set = 0, binding = 0) uniform sampler2D water_surface;

layout(push_constant) uniform PushConstants
{
	mat4 world2proj;
	uint flags;
} _pc;

void main()
{
	const ivec2 v_id =vertex_id[0];
	const vec2 inv_dim = 1.0 / vec2(textureSize(water_surface, 0));
	
	const vec4 wl = textureGather(water_surface, (vec2(v_id) + 0.5) * inv_dim); 

	const float water_scale = 2.0;

	const vec2 base_v_pos = (vec2(v_id) * inv_dim - 0.5) * water_scale;

	const vec3 a = vec3(base_v_pos, wl.w);
	const vec3 b = vec3(base_v_pos + vec2(0, 1) * inv_dim * water_scale, wl.x);
	const vec3 c = vec3(base_v_pos + vec2(1, 0) * inv_dim * water_scale, wl.z);
	const vec3 d = vec3(base_v_pos + vec2(1, 1) * inv_dim * water_scale, wl.y);

	
	{
		gl_Position = _pc.world2proj * vec4(a, 1);
		world_position = a;
		EmitVertex();
	}

	{
		gl_Position = _pc.world2proj * vec4(b, 1);
		world_position = b;
		EmitVertex();
	}

	{
		gl_Position = _pc.world2proj * vec4(c, 1);
		world_position = c;
		EmitVertex();
	}

	{
		gl_Position = _pc.world2proj * vec4(d, 1);
		world_position = d;
		EmitVertex();
	}

	EndPrimitive();
}