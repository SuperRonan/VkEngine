#pragma once

#include "string.glsl"
#include <ShaderLib:/common.glsl>

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
	vec4 position;
	DebugBufferVec4 color;
	DebugBufferVec4 back_color;
	DebugBufferVec2 glyph_size;
	uint layer;
	uint len;
	uint flags;
};

struct BufferString
{
	BufferStringMeta meta;
	// Store in uvec4 for 128 bit memory transactions?
	uint32_t data[BUFFER_STRING_PACKED_CAPACITY];
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

#define DEBUG_ENABLE_MASK	0x1
#define DEBUG_ENABLE_BIT	0x1

#define DEBUG_SPACE_MASK		(0x3 << 1)
#define DEBUG_CLIP_SPACE_BIT	(0x0 << 1)
#define DEBUG_UV_SPACE_BIT		(0x1 << 1)
#define DEBUG_PIXEL_SPACE_BIT	(0x2 << 1)

#ifndef BIND_DEBUG_BUFFERS
#define BIND_DEBUG_BUFFERS (I_WANT_TO_DEBUG & GLOBAL_ENABLE_GLSL_DEBUG)
#endif

#if BIND_DEBUG_BUFFERS

#ifndef DEBUG_BUFFER_BINDING
#define DEBUG_BUFFER_BINDING COMMON_DESCRIPTOR_BINDING
#endif

#if DEBUG_BUFFER_ACCESS_readonly
#define DEBUG_BUFFER_ACCESS readonly
#endif

#ifndef DEBUG_BUFFER_ACCESS
#define DEBUG_BUFFER_ACCESS
#endif

// sizeof: 4 * 4 * u32
struct DebugBufferHeader
{
	uint max_strings;
	uint max_lines;
	uint pad1;
	uint pad2;
	// Store the two counters on a separate cache line for now (TODO test the perf)
	uint strings_counter;
	uint ppad1;
	uint ppad2;
	uint ppad3;
	
	uint lines_counter;
	uint ppad4;
	uint ppad5;
	uint ppad6;
	
	uvec4 pad;
};

// TODO maybe store the capacities in a separate UBO (maybe even inline UBO)?
layout(DEBUG_BUFFER_BINDING + 0) restrict DEBUG_BUFFER_ACCESS buffer DebugStringBuffer
{
	DebugBufferHeader header;
	BufferString strings
#ifdef DEBUG_BUFFER_STRINGS_CAPACITY
		[DEBUG_BUFFER_STRINGS_CAPACITY];
#else 
		[];
#endif 
} _debug;

layout(DEBUG_BUFFER_BINDING + 1) restrict DEBUG_BUFFER_ACCESS buffer DebugLinesBuffer
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
	const uint m = _debug.header.max_strings;
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
	const uint m = _debug.header.max_lines;
#else 
	const uint m = 0;
#endif
	return m;
}

#ifndef DEBUG_BUFFER_ASSUME_Po2_CAPACITIES
#define DEBUG_BUFFER_ASSUME_Po2_CAPACITIES 1
#endif

uint allocateDebugStrings(uint n)
{
#if BIND_DEBUG_BUFFERS && !DEBUG_BUFFER_ACCESS_readonly
#ifdef DEBUG_BUFFER_STRINGS_CAPACITY
	const uint m = DEBUG_BUFFER_STRINGS_CAPACITY;
#else
	const uint m = _debug.header.max_strings;
#endif
#if DEBUG_BUFFER_ASSUME_Po2_CAPACITIES
	return atomicAdd(_debug.header.strings_counter, n) & (m - 1);
#else
	return atomicAdd(_debug.header.strings_counter, n) % m;
#endif
#endif
	return 0;
}

uint allocateDebugString()
{
	return allocateDebugStrings(1);
}

uint allocateDebugLines(uint n)
{
#if BIND_DEBUG_BUFFERS && !DEBUG_BUFFER_ACCESS_readonly
#ifdef DEBUG_BUFFER_LINES_CAPACITY
	const uint m = DEBUG_BUFFER_LINES_CAPACITY;
#else
	const uint m = _debug.header.max_lines;
#endif
#if DEBUG_BUFFER_ASSUME_Po2_CAPACITIES
	return atomicAdd(_debug.header.lines_counter, n) & (m - 1);
#else
	return atomicAdd(_debug.header.lines_counter, n) % m;
#endif
#endif
	return 0;
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

#ifndef GLYPH_SIZE_PIX
#define GLYPH_SIZE_PIX vec2(16, 16)
#endif

#define GLYPH_SIZE_TINY 0
#define GLYPH_SIZE_SMALL 1
#define GLYPH_SIZE_NORMAL 2
#define GLYPH_SIZE_LARGE 3
#define GLYPH_SIZE_HUGE 4

#ifndef GLYPH_SIZE
#define GLYPH_SIZE GLYPH_SIZE_NORMAL
#endif

float getGlyphSizeMult()
{
#if GLYPH_SIZE == GLYPH_SIZE_TINY
	return 0.25f;
#elif GLYPH_SIZE == GLYPH_SIZE_SMALL
	return 0.5f;
#elif GLYPH_SIZE == GLYPH_SIZE_NORMAL
	return 1.0f;
#elif GLYPH_SIZE == GLYPH_SIZE_LARGE
	return 2.0f;
#elif GLYPH_SIZE == GLYPH_SIZE_HUGE
	return 4.0f;
#endif
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
	meta.position = c.pos;
	meta.layer = c.layer;
	meta.len = getShaderStringLength(str);
	meta.glyph_size = DebugBufferVec2(glyph_size);
	meta.color = DebugBufferVec4(ft_color);
	meta.back_color = DebugBufferVec4(bg_color);
	meta.flags = flags;

	const uint index = allocateDebugString();

	_debug.strings[index].meta = meta;
	for(int i = 0; i < (meta.len + 3) / 4; ++i)
	{
		_debug.strings[index].data[i] = str.data[i];
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