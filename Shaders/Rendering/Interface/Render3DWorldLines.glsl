#version 460

#include <ShaderLib:/common.glsl>

#include "common.glsl"

void swap(inout vec3 a, inout vec3 b)
{
	vec3 c = a;
	a = b;
	b = c;
}

mat3 BasisFromAxis(uint axis, bool direction)
{
	mat3 res = BasisFromAxis(axis);
	if(direction)
	{
		swap(res[0], res[1]);
	}
	return res;
}

#if SHADER_SEMANTIC_VERTEX

layout(location = 0) out uint vid;

void main()
{
	vid = gl_VertexIndex;
}

#endif


#if SHADER_SEMANTIC_GEOMETRY

layout(points) in;
layout(location = 0) in uint vid[1];

layout(line_strip, max_vertices=2) out;
layout(location = 0) out vec4 frag_color;

void main()
{
	uint global_index = vid[0];
	const bool direction = (global_index & 0x1) != 0; 
	global_index >>= 1;
	const uint id_in_plane = global_index / 3;
	const uint axis_id = global_index % 3;

	const uint axis_mask = GetAxisMask();
	if((axis_mask & (1 << axis_id)) != 0)
	{
		const mat3 basis = BasisFromAxis(axis_id, direction);

		const mat4 w2p = _pc.world_to_proj;
		
		vec4 color = vec4(1..xxx, 0.5);

		const float u = (float(id_in_plane) * _pc.oo_line_count_minus_one) * 2.0f - 1.0f;

		if(id_in_plane == GetLineCount())
		{
			color.xyz *= basis[1];
			color.a = 1.0f;
		}

		vec2 p0 = vec2(u, -1);
		vec2 p1 = vec2(u, 1);

		gl_Position = w2p * vec4(basis * vec3(p0, 0), 1);
		frag_color = color;
		EmitVertex();

		gl_Position = w2p * vec4(basis * vec3(p1, 0), 1);
		frag_color = color;
		EmitVertex();

		EndPrimitive();
	}
}

#endif


#if SHADER_SEMANTIC_FRAGMENT

layout(location = 0) in vec4 in_color;
layout(location = 0) out vec4 out_color;

void main()
{
	out_color = in_color;
}

#endif