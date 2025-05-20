#version 460

#define I_WANT_TO_DEBUG 1
#define DEBUG_BUFFER_ACCESS_readonly 1

#include <ShaderLib:/common.glsl>
#include "debugBuffers.glsl"

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

struct Varying
{
	vec2 uv;
	vec4 ft_color;
	vec4 bg_color;
};

#if SHADER_SEMANTIC_GEOMETRY

layout(points) in;

layout(location = 0) in uint vid[1];

layout(triangle_strip, max_vertices=4) out;


layout(location = 0) out Varying v_out;
layout(location = 3) out flat uint v_string_index;

layout(push_constant) uniform PushConstantBinding
{
	PushConstant pc;
};

void main()
{
#if GLOBAL_ENABLE_GLSL_DEBUG
	const uint index = vid[0];
	const uint len = (_debug_strings_meta.meta[index].len);
	const bool emit = (index < _debug.header.strings_counter) && (len != 0);
	if(emit)
	{
		const BufferStringMeta meta = _debug_strings_meta.meta[index];
		const uint flags = meta.flags;
		const uint coord_space_flags = meta.flags & DEBUG_SPACE_MASK;

		vec4 position = vec4(meta.position, 1);
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
			v_string_index = meta.content_index;
			v_out.uv = vec2(0, 0);
			v_out.ft_color = meta.color;
			v_out.bg_color = meta.back_color;
			gl_Position = vec4(tl, position.zw);
			EmitVertex();
		}

		{
			v_string_index = meta.content_index;
			v_out.uv = vec2(0, 1);
			v_out.ft_color = meta.color;
			v_out.bg_color = meta.back_color;
			gl_Position = vec4(bl, position.zw);
			EmitVertex();
		}

		{
			v_string_index = meta.content_index;
			v_out.uv = vec2(len, 0);
			v_out.ft_color = meta.color;
			v_out.bg_color = meta.back_color;
			gl_Position = vec4(tr, position.zw);
			EmitVertex();
		}

		{
			v_string_index = meta.content_index;
			v_out.uv = vec2(len, 1);
			v_out.ft_color = meta.color;
			v_out.bg_color = meta.back_color;
			gl_Position = vec4(br, position.zw);
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
	const uint len = emit_string ? (_debug_strings_meta.meta[index].len) : 0;

	emit_string = emit_string && (index < _debug.header.strings_counter);
	emit_string = emit_string && (len > 0);
	
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
		

		const BufferStringMeta meta = _debug_strings_meta.meta[index];
		const uint coord_space_flags = meta.flags & DEBUG_SPACE_MASK;

		vec4 position = vec4(meta.position, 1);
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
		v_string_index[base_primitive_id + 0] = meta.content_index;
		v_string_index[base_primitive_id + 1] = meta.content_index;
	}
#else
	SetMeshOutputsEXT(0, 0);
#endif
}

#endif

#if SHADER_SEMANTIC_FRAGMENT


layout(SHADER_DESCRIPTOR_BINDING + 0) uniform sampler2DArray glyphs;

layout(location = 0) in Varying v_in;
#if MESH_PIPELINE
#extension GL_EXT_mesh_shader : require
layout(location = 3) in perprimitiveEXT flat uint v_string_index;
#elif GEOMETRY_PIPELINE
layout(location = 3) in flat uint v_string_index;
#else
#error "Unknown pipeline"
#endif

layout(location = 0) out vec4 o_color;

void main()
{
#if GLOBAL_ENABLE_GLSL_DEBUG
	const vec2 uv_in_str = v_in.uv;
	const uint char_index = uint(uv_in_str.x);
	const uint chunk_index = char_index / 4;
	const uint char_chunk = _debug_strings_content.chunks[v_string_index + chunk_index];
	const uint ascii = getCharInChunk(char_chunk, char_index % 4);
	const vec2 uv_in_char = vec2(uv_in_str.x - floor(uv_in_str.x), uv_in_str.y);

	float glyph_texel = textureLod(glyphs, vec3(uv_in_char, ascii), 0).x;

	o_color = lerp(v_in.bg_color, v_in.ft_color, glyph_texel);
#endif
}

#endif

