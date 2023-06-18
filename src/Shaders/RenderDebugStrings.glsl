#version 460

#define I_WANT_TO_DEBUG 1
#define DEBUG_BUFFER_ACCESS_readonly 1

#include "common.glsl"
#include "interop_cpp.glsl"
#include "DebugBuffers.glsl"

#if SHADER_SEMANTIC_VERTEX

#endif

struct PushConstant
{
	uvec3 resolution;
	vec2 oo_resolution;
};


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

layout(location = 0) out flat uint v_string_index;
layout(location = 1) out vec2 v_uv;
layout(location = 2) out vec4 v_ft_color;
layout(location = 3) out vec4 v_bg_color;

layout(push_constant) uniform PushConstantBinding
{
	PushConstant pc;
};

void main()
{
#if GLOBAL_ENABLE_GLSL_DEBUG
	const uint index = vid[0];
	const uint len = (_debug.strings[index].meta.len);
	if(len != 0)
	{
		const uint flags = _debug.strings[index].meta.flags;
		const BufferStringMeta meta = _debug.strings[index].meta;
		const uint coord_space_flags = meta.flags & DEBUG_SPACE_MASK;

		vec2 position = meta.position;
		vec2 gs = meta.glyph_size;
		if(coord_space_flags == DEBUG_UV_SPACE_BIT || coord_space_flags == DEBUG_PIXEL_SPACE_BIT)
		{
			if(coord_space_flags == DEBUG_PIXEL_SPACE_BIT)
			{
				position = (position + 0.5f) * pc.oo_resolution;
				gs = gs * pc.oo_resolution;
			}
			
			position = UVToClipSpace(position);
			gs *= 2;
		}


		const vec2 tl = position;
		const vec2 tr = tl + vec2(len * gs.x, 0);
		const vec2 br = tr + vec2(0, gs.y);
		const vec2 bl = tl + vec2(0, gs.y);

		


		{
			v_string_index = index;
			v_uv = vec2(0, 0);
			v_ft_color = meta.color;
			v_bg_color = meta.back_color;
			gl_Position = vec4(tl, 0.5, 1);
			EmitVertex();
		}

		{
			v_string_index = index;
			v_uv = vec2(0, 1);
			v_ft_color = meta.color;
			v_bg_color = meta.back_color;
			gl_Position = vec4(bl, 0.5, 1);
			EmitVertex();
		}

		{
			v_string_index = index;
			v_uv = vec2(len, 0);
			v_ft_color = meta.color;
			v_bg_color = meta.back_color;
			gl_Position = vec4(tr, 0.5, 1);
			EmitVertex();
		}

		{
			v_string_index = index;
			v_uv = vec2(len, 1);
			v_ft_color = meta.color;
			v_bg_color = meta.back_color;
			gl_Position = vec4(br, 0.5, 1);
			EmitVertex();
		}

		EndPrimitive();
	}
#endif
}

#endif

#if SHADER_SEMANTIC_FRAGMENT

#include "random.glsl"

layout(set = 1, binding = 0) uniform sampler2DArray glyphs;

layout(location = 0) in flat uint v_string_index;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec4 v_ft_color;
layout(location = 3) in vec4 v_bg_color;

layout(location = 0) out vec4 o_color;

void main()
{
#if GLOBAL_ENABLE_GLSL_DEBUG
	const vec2 uv_in_str = v_uv;
	const uint char_index = uint(uv_in_str.x);
	const uint char_chunk = _debug.strings[v_string_index].data[char_index / 4];
	const uint ascii = getCharInChunk(char_chunk, char_index % 4);
	const vec2 uv_in_char = vec2(uv_in_str.x - floor(uv_in_str.x), uv_in_str.y);

	float glyph_texel = textureLod(glyphs, vec3(uv_in_char, ascii), 0).x;

	o_color = lerp(v_bg_color, v_ft_color, glyph_texel);

	uint rng = char_index;
	// o_color.xyz = randomRGB(rng);
	//o_color.xy = uv_in_char;
	// o_color.z = 0;
#endif
}

#endif