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
	if(index < _debug.header.lines_counter)
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
#define MAX_VERTICES (LOCAL_SIZE_X * 2)
#define MAX_PRIMITIVES (LOCAL_SIZE_X * 1)

layout(location = 0) out vec4 v_color[];

layout(push_constant) uniform PushConstantBinding
{
	PushConstant pc;
};

layout(local_size_x = LOCAL_SIZE_X) in;
layout(lines, max_vertices = MAX_VERTICES, max_primitives = MAX_PRIMITIVES) out;

void main()
{
#if GLOBAL_ENABLE_GLSL_DEBUG
	const uint lid = gl_LocalInvocationIndex.x;
	const uint wid = gl_WorkGroupID.x;
	const uint gid = gl_GlobalInvocationID.x;

	bool emit_line = gid < _debug.header.lines_counter;

	const uint index = gid;
	BufferDebugLine line; 
	
	if(emit_line)
	{
		line = _debug_lines.lines[index];
		emit_line = line.flags != 0;
	}
	
	
	const uint emit_line_ui = emit_line ? 1 : 0;
	const uint num_lines = subgroupAdd(emit_line_ui);
	//subgroupBarrier();
	if(subgroupElect())
	{
		SetMeshOutputsEXT(num_lines * 2, num_lines);
	}

	if(emit_line)
	{
		const uint subgroup_line_id = subgroupExclusiveAdd(emit_line_ui);
		//subgroupBarrier();
		const uint base_vert_id = subgroup_line_id * 2;
		const uint base_primitive_id = subgroup_line_id;
		
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
			v_color[base_vert_id + 0] = line.color1;
			gl_MeshVerticesEXT[base_vert_id + 0].gl_Position = p1;

			v_color[base_vert_id + 1] = line.color2;
			gl_MeshVerticesEXT[base_vert_id + 1].gl_Position = p2;
		}


		gl_PrimitiveLineIndicesEXT[base_primitive_id + 0] = uvec2(base_vert_id + 0, base_vert_id + 1);
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

