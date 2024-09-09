#version 460

#include <ShaderLib:/common.glsl>

#include "common.glsl"

#ifndef RENDER_3D_WORLD_PLANES_CHECKERBOARD
#define RENDER_3D_WORLD_PLANES_CHECKERBOARD 1
#endif



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

layout(triangle_strip, max_vertices=4) out;
layout(location = 0) out vec4 frag_color;
#if RENDER_3D_WORLD_PLANES_CHECKERBOARD
layout(location = 1) out vec2 v_uv;
#endif

void main()
{
	const uint axis = clamp(vid[0], 0, 3);

	const uint axis_mask = GetAxisMask();
	if((axis_mask & (1 << axis)) != 0)
	{
		const mat3 basis = BasisFromAxis(axis);

		const mat4 w2p = _pc.world_to_proj;


		vec4 color = vec4(1, 1, 1, 0.05);


		for(uint i = 0; i < 4; ++i)
		{
			const uint ix = i % 2;
			const uint iy = i / 2;
			const vec2 uv = vec2(ix, iy) * 2.0f - 1.0f;
	#if RENDER_3D_WORLD_PLANES_CHECKERBOARD
			v_uv = uv;
	#endif
			const vec3 p = basis * vec3(uv, 0);
			gl_Position = w2p * vec4(p, 1);
			frag_color = color;
			EmitVertex();	
		}
		
		EndPrimitive();
	}
}

#endif


#if SHADER_SEMANTIC_FRAGMENT

layout(location = 0) in vec4 in_color;
#if RENDER_3D_WORLD_PLANES_CHECKERBOARD
layout(location = 1) in vec2 in_uv;
#endif
layout(location = 0) out vec4 out_color;

void main()
{
	vec4 color = in_color;
#if RENDER_3D_WORLD_PLANES_CHECKERBOARD
	vec2 uv = in_uv + 1..xx;
	uv *= ((_pc.line_count_f - 1) / 2);
	ivec2 uvi = ivec2(uv);
	bool cb = (((uvi.x) ^ (uvi.y)) & 1) == 0;
	const float mult = 0.85;
	if(cb)
	{
		color.a *= mult;
	}
	else
	{
		color.a /= mult;
	}
#endif

	out_color = color;
}

#endif