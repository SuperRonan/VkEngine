#pragma once

#include "string.glsl"
#include <ShaderLib:/common.glsl>
#include "DebugBuffersDefinitions.h"

#include <ShaderLib:/CommonUBO.glsl>

#ifndef DEBUG_ENABLE_DEBUG_GLOBAL_SIGNAL
#define DEBUG_ENABLE_DEBUG_GLOBAL_SIGNAL 0
#endif 

#if DEBUG_ENABLE_DEBUG_GLOBAL_SIGNAL
bool _g_debug_signal = false;
#endif

// Should be constant accross all shaders (contrary to the shader string capacity)
#ifndef BUFFER_STRING_CAPACITY
// In Number of uint32_t
#define BUFFER_STRING_CAPACITY 64 
#endif

#if BUFFER_STRING_CAPACITY < SHADER_STRING_CAPACITY
#error "BUFFER_STRING_CAPACITY should be strictly bigger than SHADER_STRING_CAPACITY" 
#endif

#define BUFFER_STRING_PACKED_CAPACITY (BUFFER_STRING_CAPACITY / 4)

// TODO if not available: use packHalf2x16
#if SHADER_FP16_AVAILABLE && SHADER_SSBO_16BITS_ACCESS
#define DebugBufferVec4 fp16vec4IFP
#define DebugBufferVec2 fp16vec2IFP
#else 
#define DebugBufferVec4 vec4
#define DebugBufferVec2 vec2
#endif

// TODO optimize memory (mainly fp16 for colors)
struct BufferStringMeta
{
	vec3 position;
	uint layer;

	DebugBufferVec4 color;
	DebugBufferVec4 back_color;
	
	DebugBufferVec2 glyph_size;
	uint len;
	uint flags;
	uint content_index;
};

struct BufferDebugLine
{
	vec4 p1;
	vec4 p2;
	DebugBufferVec4 color1;
	DebugBufferVec4 color2;
	uint layer;
	uint flags;
};





#if BIND_DEBUG_BUFFERS



#if DEBUG_BUFFER_ACCESS_readonly
#define DEBUG_BUFFER_ACCESS readonly
#endif

#ifndef DEBUG_BUFFER_ACCESS
#define DEBUG_BUFFER_ACCESS
#endif

// sizeof: 3 * 4 * u32
struct DebugBuffersHeader
{
	uint strings_counter;
	uint pad_1_0;
	uint pad_1_1;
	uint pad_1_2;

	uint chunks_counter;
	uint pad_2_0;
	uint pad_2_1;
	uint pad_2_2;

	uint lines_counter;
	uint pad_3_0;
	uint pad_3_1;
	uint pad_3_2;
};

// TODO maybe store the capacities in a separate UBO (maybe even inline UBO)?
layout(DEBUG_BUFFER_BINDING + 0) restrict DEBUG_BUFFER_ACCESS buffer DebugBufferHeaderBinding
{
	DebugBuffersHeader header; 
} _debug;

layout(DEBUG_BUFFER_BINDING + 1) restrict DEBUG_BUFFER_ACCESS buffer DebugStringMetaBinding
{
	BufferStringMeta meta
#ifdef DEBUG_BUFFER_STRINGS_CAPACITY
	[DEBUG_BUFFER_STRINGS_CAPACITY];
#else
	[];
#endif
} _debug_strings_meta;

layout(DEBUG_BUFFER_BINDING + 2) restrict DEBUG_BUFFER_ACCESS buffer DebugStringContentBinding
{
	uint chunks[];
} _debug_strings_content;

layout(DEBUG_BUFFER_BINDING + 3) restrict DEBUG_BUFFER_ACCESS buffer DebugLinesBuffer
{
	BufferDebugLine lines
#ifdef DEBUG_BUFFER_LINES_CAPACITY
		[DEBUG_BUFFER_LINES_CAPACITY];
#else
		[];
#endif
} _debug_lines;

#endif

uint debugStringsCapacity()
{
#ifdef DEBUG_BUFFER_STRINGS_CAPACITY
	const uint m = DEBUG_BUFFER_STRINGS_CAPACITY;
#elif BIND_DEBUG_BUFFERS && !DEBUG_BUFFER_ACCESS_writeonly 
	const uint m = _common_ubo.debug_buffer_max_strings_mask + 1;
#else 
	const uint m = 0;
#endif
	return m;
}

uint debugLinesCapacity()
{
#ifdef DEBUG_BUFFER_LINES_CAPACITY
	const uint m = DEBUG_BUFFER_LINES_CAPACITY;
#elif BIND_DEBUG_BUFFERS && !DEBUG_BUFFER_ACCESS_writeonly
	const uint m = _common_ubo.debug_buffer_max_lines_mask + 1;
#else 
	const uint m = 0;
#endif
	return m;
}

uint allocateDebugStrings(uint n)
{
	uint res = 0;
#if BIND_DEBUG_BUFFERS && !DEBUG_BUFFER_ACCESS_readonly
	const uint m = debugStringsCapacity() - 1;
	res = atomicAdd(_debug.header.strings_counter, n);
	res = res & m;
#endif
	return res;
}

uint allocateDebugString()
{
	return allocateDebugStrings(1);
}

// len in chunks 
uint allocateDebugStringContent(uint len)
{
#if BIND_DEBUG_BUFFERS && !DEBUG_BUFFER_ACCESS_readonly
	return atomicAdd(_debug.header.chunks_counter, len);
#endif
	return 0;
}

uint allocateDebugLines(uint n)
{
	uint res = 0;
#if BIND_DEBUG_BUFFERS && !DEBUG_BUFFER_ACCESS_readonly
	const uint m = debugLinesCapacity() - 1;
	res = atomicAdd(_debug.header.lines_counter, n);
	res = res & m;
#endif
	return res;
}

uint allocateDebugLine()
{
	return allocateDebugLines(1);
}

struct DebugStringCaret
{
	vec4 pos;
	uint layer;
};

DebugStringCaret Caret2D(vec2 pos, uint layer)
{
	return DebugStringCaret(vec4(pos, 0, 1), layer);
}

DebugStringCaret Caret3D(vec4 pos, uint layer)
{
	return DebugStringCaret(pos, layer);
}

#define Caret DebugStringCaret



float getGlyphSizeMult()
{
	return exp2(float(GLYPH_SIZE - GLYPH_SIZE_NORMAL));
}

vec2 debugStringDefaultGlyphSizePix()
{
	return GLYPH_SIZE_PIX * getGlyphSizeMult();
}

vec2 debugStringDefaultGlyphSizeUV()
{
	const vec2 screen_res = vec2(1600, 900);
	return debugStringDefaultGlyphSizePix() * rcp(screen_res);
}

vec2 debugStringDefaultGlyphSizeClipSpace()
{
	return debugStringDefaultGlyphSizeUV() * 2.0f;
}

vec2 debugStringDefaultGlyphSize(uint flags)
{
	flags = flags & DEBUG_SPACE_MASK; 
	if(flags == DEBUG_PIXEL_SPACE_BIT)		return debugStringDefaultGlyphSizePix();
	else if (flags == DEBUG_UV_SPACE_BIT)	return debugStringDefaultGlyphSizeUV();
	else									return debugStringDefaultGlyphSizeClipSpace();
}

vec4 debugStringDefaultFrontColor()
{
	return vec4(1);
}

vec4 debugStringDefaultBackColor()
{
	return vec4(0..xxx, 0.05);
}

vec4 debugDimColor(uint index)
{
	vec4 res = vec4(0..xxx, 1);
	res[index] = 1;
	if(index == 2)	res.xy = 0.25.xx; // blue can be hard to read else
	else if(index == 3)	res = 0.9.xxxx;
	return res;
}

Caret pushToDebug(const in ShaderString str, Caret c, bool ln, vec4 ft_color, vec4 bg_color, vec2 glyph_size, uint flags)
{
#if BIND_DEBUG_BUFFERS && !DEBUG_BUFFER_ACCESS_readonly
	BufferStringMeta meta;
	meta.position = c.pos.xyz / c.pos.w;
	meta.layer = c.layer;
	meta.len = getShaderStringLength(str);
	meta.glyph_size = DebugBufferVec2(glyph_size);
	meta.color = DebugBufferVec4(ft_color);
	meta.back_color = DebugBufferVec4(bg_color);
	meta.flags = flags;

	const uint index = allocateDebugString();
	const uint needed_chunks = (meta.len + 3) / 4;
	const uint chunk_index = allocateDebugStringContent(needed_chunks);
	meta.content_index = chunk_index;

	_debug_strings_meta.meta[index] = meta;
	for(int i = 0; i < needed_chunks; ++i)
	{
		_debug_strings_content.chunks[i + chunk_index] = str.data[i];
	}
	
	Caret res = c;
	if(ln)
	{
		res.pos.y += glyph_size.y;
	}
	else
	{
		res.pos.x += glyph_size.x * meta.len;
	}
	return res;
#else
	return c;
#endif
}

Caret pushToDebug(uint n, Caret c, bool ln, vec4 ft_color, vec4 bg_color, vec2 glyph_size, uint flags)
{
	return pushToDebug(toStr(n), c, ln, ft_color, bg_color, glyph_size, flags);
}

Caret pushToDebug(int n, Caret c, bool ln, vec4 ft_color, vec4 bg_color, vec2 glyph_size, uint flags)
{
	return pushToDebug(toStr(n), c, ln, ft_color, bg_color, glyph_size, flags);
}

Caret pushToDebug(float f, Caret c, bool ln, vec4 ft_color, vec4 bg_color, vec2 glyph_size, uint flags)
{
	return pushToDebug(toStr(f), c, ln, ft_color, bg_color, glyph_size, flags);
}


#define DECLARE_pushToDebug_gvec(vecType, N) Caret pushToDebug(const in vecType##N v, Caret c, bool ln, vec4 ft_color, vec4 bg_color, vec2 glyph_size, uint flags) { for(int i=0; i<N; ++i)	{ c = pushToDebug(v[i], c, ln, debugDimColor(i), bg_color, glyph_size, flags); } return c; }

#define DECLARE_pushToDebug_gvec_all_dims(vecType) DECLARE_pushToDebug_gvec(vecType, 2) DECLARE_pushToDebug_gvec(vecType, 3) DECLARE_pushToDebug_gvec(vecType, 4)

#define DECLARE_pushToDebug_gvec_all_types DECLARE_pushToDebug_gvec_all_dims(vec) DECLARE_pushToDebug_gvec_all_dims(uvec) DECLARE_pushToDebug_gvec_all_dims(ivec)

DECLARE_pushToDebug_gvec_all_types



#define DEBUG_SPACE_FLAG_Pix DEBUG_PIXEL_SPACE_BIT
#define DEBUG_SPACE_FLAG_UV DEBUG_UV_SPACE_BIT
#define DEBUG_SPACE_FLAG_ClipSpace DEBUG_CLIP_SPACE_BIT
#define GET_DEBUG_SPACE_FLAG(Space) DEBUG_SPACE_FLAG_##Space

// Wish I had templates

#define DECLARE_pushToDebug_space_type_param_eol_color(Type, Space) Caret pushToDebug##Space(const in Type t, Caret c, bool ln, vec4 color, vec4 bg_color) { 	return pushToDebug(t, c, ln, color, bg_color, debugStringDefaultGlyphSize(GET_DEBUG_SPACE_FLAG(Space)), GET_DEBUG_SPACE_FLAG(Space)); } Caret pushToDebug##Space(const in Type t, Caret c, bool ln, vec4 color) { return pushToDebug##Space(t, c, ln, color, debugStringDefaultBackColor()); } 

#define DECLARE_pushToDebug_space_type_eol_color(Type, Space, eol) Caret pushToDebug##Space##eol(const in Type t, Caret c, vec4 color, vec4 bg_color) { return pushToDebug##Space(t, c, true, color, bg_color);} Caret pushToDebug##Space##eol(const in Type t, Caret c, vec4 color) { return pushToDebug##Space(t, c, true, color);}
#define DECLARE_pushToDebug_space_type_eol(Type, Space, eol) Caret pushToDebug##Space##eol(const in Type t, Caret c) { return pushToDebug##Space(t, c, true, debugStringDefaultFrontColor());}

#define DECLARE_pushToDebug_space_type_eol_2(Type, Space, eol) DECLARE_pushToDebug_space_type_eol_color(Type, Space, eol) DECLARE_pushToDebug_space_type_eol(Type, Space, eol)

#define DECLARE_pushToDebug_space_type_all_eol(Type, Space) DECLARE_pushToDebug_space_type_param_eol_color(Type, Space) DECLARE_pushToDebug_space_type_eol_2(Type, Space, Ln) DECLARE_pushToDebug_space_type_eol_2(Type, Space, Sl)

#define DECLARE_pushToDebug_type_all_space(Type) DECLARE_pushToDebug_space_type_all_eol(Type, Pix) DECLARE_pushToDebug_space_type_all_eol(Type, UV) DECLARE_pushToDebug_space_type_all_eol(Type, ClipSpace)

#define DECLARE_pushToDebug_type_all_vec_dims(vecType) DECLARE_pushToDebug_type_all_space(vecType##2) DECLARE_pushToDebug_type_all_space(vecType##3) DECLARE_pushToDebug_type_all_space(vecType##4)

#define DECLARE_pushToDebug_type_all_vec DECLARE_pushToDebug_type_all_vec_dims(vec) DECLARE_pushToDebug_type_all_vec_dims(ivec) DECLARE_pushToDebug_type_all_vec_dims(uvec)

#define DECLARE_pushToDebug_all_type DECLARE_pushToDebug_type_all_space(ShaderString) DECLARE_pushToDebug_type_all_space(uint) DECLARE_pushToDebug_type_all_space(int) DECLARE_pushToDebug_type_all_space(float)


DECLARE_pushToDebug_all_type
DECLARE_pushToDebug_type_all_vec




void pushDebugLine(vec4 p1, vec4 p2, uint layer, vec4 c1, vec4 c2, uint flags)
{
#if BIND_DEBUG_BUFFERS && !DEBUG_BUFFER_ACCESS_readonly
	const uint index = allocateDebugLine();
	BufferDebugLine line;
	line.p1 = p1;
	line.p2 = p2;
	line.color1 = DebugBufferVec4(c1);
	line.color2 = DebugBufferVec4(c2);
	line.layer = layer;
	line.flags = line.flags | 0x1;
	_debug_lines.lines[index] = line;
#endif
}

void pushDebugLineUV(vec2 p1, vec2 p2, uint layer, vec4 c1, vec4 c2, uint flags)
{
	flags |= DEBUG_UV_SPACE_BIT;
	pushDebugLine(vec4(p1, 0, 1), vec4(p2, 0, 1), layer, c1, c2, flags);
}

void pushDebugLinePix(vec2 p1, vec2 p2, uint layer, vec4 c1, vec4 c2, uint flags)
{
	flags |= DEBUG_PIXEL_SPACE_BIT;
	pushDebugLine(vec4(p1, 0, 1), vec4(p2, 0, 1), layer, c1, c2, flags);
}

void pushDebugLine(mat4 xform, vec3 p1, vec3 p2, uint layer, vec4 c1, vec4 c2, uint flags)
{
	const vec4 a = xform * vec4(p1, 1);
	const vec4 b = xform * vec4(p2, 1);
	flags |= DEBUG_CLIP_SPACE_BIT;
	pushDebugLine(a, b, layer, c1, c2, flags);
}


void pushDebugLineUV(vec2 p1, vec2 p2, uint layer, vec3 c1, vec3 c2, uint flags)
{
	pushDebugLineUV(p1, p2, layer, vec4(c1, 1), vec4(c2, 1), flags);
}

void pushDebugLinePix(vec2 p1, vec2 p2, uint layer, vec3 c1, vec3 c2, uint flags)
{
	pushDebugLinePix(p1, p2, layer, vec4(c1, 1), vec4(c2, 1), flags);
}

void pushDebugLine(mat4 xform, vec3 p1, vec3 p2, uint layer, vec3 c1, vec3 c2, uint flags)
{
	pushDebugLine(xform, p1, p2, layer, vec4(c1, 1), vec4(c2, 1), flags);
}