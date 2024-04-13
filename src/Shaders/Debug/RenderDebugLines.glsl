#version 460

#define I_WANT_TO_DEBUG 1
#define DEBUG_BUFFER_ACCESS_readonly 1

#include <ShaderLib:/common.glsl>

#include "DebugBuffers.glsl"

#if SHADER_SEMANTIC_VERTEX

#endif

struct PushConstant
{
	uvec3 resolution;
	vec2 oo_resolution;
};

#ifndef USE_STRUCT_VARYING
#define USE_STRUCT_VARYING 0
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

layout(line_strip, max_vertices=2) out;

layout(location = 0) out vec4 v_color;
//layout(location = 1) out vec4 postion;

layout(push_constant) uniform PushConstantBinding
{
	PushConstant pc;
};

void main()
{
#if GLOBAL_ENABLE_GLSL_DEBUG
	const uint index = vid[0];
	const BufferDebugLine line = _debug_lines.lines[index];
	if(line.flags != 0)
	{
		const uint coord_space_flags = line.flags & DEBUG_SPACE_MASK;

		vec4 p1 = line.p1;
		vec4 p2 = line.p2;
		
		if(coord_space_flags == DEBUG_UV_SPACE_BIT || coord_space_flags == DEBUG_PIXEL_SPACE_BIT)
		{
			if(coord_space_flags == DEBUG_PIXEL_SPACE_BIT)
			{
				p1.xy = (p1.xy + 0.5f) * pc.oo_resolution;
				p2.xy = (p2.xy + 0.5f) * pc.oo_resolution;
			}
			p1.xy = UVToClipSpace(p1.xy);
			p2.xy = UVToClipSpace(p2.xy);
			// On Screen
			p1.zw = vec2(0, 1);
			p2.zw = vec2(0, 1);
		}

		{
			v_color = line.color1;
			gl_Position = p1;
			EmitVertex();
		}

		{
			v_color = line.color2;
			gl_Position = p2;
			EmitVertex();
		}

		EndPrimitive();
	}
#endif
}

#endif

#if SHADER_SEMANTIC_MESH

#extension GL_EXT_mesh_shader : require
#extension GL_KHR_shader_subgroup_basic : require
#extension GL_KHR_shader_subgroup_ballot : require
#extension GL_KHR_shader_subgroup_arithmetic : require

#define LOCAL_SIZE_X 32
#define MAX_VERTICES (LOCAL_SIZE_X * 4)
#define MAX_PRIMITIVES (LOCAL_SIZE_X * 2)

layout(location = 0) out Varying v_out[];
layout(location = 3) out perprimitiveEXT uint v_string_index[];

layout(push_constant) uniform PushConstantBinding
{
	PushConstant pc;
};

layout(local_size_x = LOCAL_SIZE_X) in;
layout(triangles, max_vertices = MAX_VERTICES, max_primitives = MAX_PRIMITIVES) out;

void main()
{
#if GLOBAL_ENABLE_GLSL_DEBUG
	const uint lid = gl_LocalInvocationIndex.x;
	const uint wid = gl_WorkGroupID.x;
	const uint gid = gl_GlobalInvocationID.x;

	bool emit_string = gid < debugStringsCapacity();

	const uint index = gid;
	const uint len = emit_string ? (_debug.strings[index].meta.len) : 0;

	emit_string = (len > 0);
	
	const uint emit_string_ui = emit_string ? 1 : 0;
	const uint num_triangles = subgroupAdd(emit_string_ui) * 2;
	//subgroupBarrier();
	if(subgroupElect())
	{
		SetMeshOutputsEXT(num_triangles * 2, num_triangles);
	}

	if(emit_string)
	{
		const uint subgroup_string_id = subgroupExclusiveAdd(emit_string_ui);
		//subgroupBarrier();
		const uint base_vert_id = subgroup_string_id * 4;
		const uint base_primitive_id = subgroup_string_id * 2;
		

		const BufferStringMeta meta = _debug.strings[index].meta;
		const uint coord_space_flags = meta.flags & DEBUG_SPACE_MASK;

		vec4 position = meta.position;
		vec2 gs = meta.glyph_size;
		if(coord_space_flags == DEBUG_UV_SPACE_BIT || coord_space_flags == DEBUG_PIXEL_SPACE_BIT)
		{
			if(coord_space_flags == DEBUG_PIXEL_SPACE_BIT)
			{
				position.xy = (position.xy + 0.5f) * pc.oo_resolution;
				gs = gs * pc.oo_resolution;
			}
			
			position.xy = UVToClipSpace(position.xy);
			gs *= 2;

			// On Screen
			position.zw = vec2(0, 1);
		}

		const vec2 tl = position.xy;
		const vec2 tr = tl + vec2(len * gs.x, 0);
		const vec2 br = tr + vec2(0, gs.y);
		const vec2 bl = tl + vec2(0, gs.y);

		{
			// v_string_index[base_vert_id + 0] = index;
			v_out[base_vert_id + 0].uv = vec2(0, 0);
			v_out[base_vert_id + 0].ft_color = meta.color;
			v_out[base_vert_id + 0].bg_color = meta.back_color;
			gl_MeshVerticesEXT[base_vert_id + 0].gl_Position = vec4(tl, position.zw);

			// v_string_index[base_vert_id + 1] = index;
			v_out[base_vert_id + 1].uv = vec2(0, 1);
			v_out[base_vert_id + 1].ft_color = meta.color;
			v_out[base_vert_id + 1].bg_color = meta.back_color;
			gl_MeshVerticesEXT[base_vert_id + 1].gl_Position = vec4(bl, position.zw);

			// v_string_index[base_vert_id + 2] = index;
			v_out[base_vert_id + 2].uv = vec2(len, 0);
			v_out[base_vert_id + 2].ft_color = meta.color;
			v_out[base_vert_id + 2].bg_color = meta.back_color;
			gl_MeshVerticesEXT[base_vert_id + 2].gl_Position = vec4(tr, position.zw);

			// v_string_index[base_vert_id + 3] = index;
			v_out[base_vert_id + 3].uv = vec2(len, 1);
			v_out[base_vert_id + 3].ft_color = meta.color;
			v_out[base_vert_id + 3].bg_color = meta.back_color;
			gl_MeshVerticesEXT[base_vert_id + 3].gl_Position = vec4(br, position.zw); 
		}


		gl_PrimitiveTriangleIndicesEXT[base_primitive_id + 0] = uvec3(base_vert_id + 0, base_vert_id + 1, base_vert_id + 2);
		gl_PrimitiveTriangleIndicesEXT[base_primitive_id + 1] = uvec3(base_vert_id + 3, base_vert_id + 2, base_vert_id + 1);
		v_string_index[base_primitive_id + 0] = index;
		v_string_index[base_primitive_id + 1] = index;
	}
#else
	SetMeshOutputsEXT(0, 0);
#endif
}

#endif

#if SHADER_SEMANTIC_FRAGMENT



layout(location = 0) in vec4 v_color;

layout(location = 0) out vec4 o_color;

void main()
{
#if GLOBAL_ENABLE_GLSL_DEBUG
	o_color = v_color;
#endif
}

#endif

