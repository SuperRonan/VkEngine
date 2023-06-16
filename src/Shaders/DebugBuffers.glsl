#pragma once

#include "String.glsl"

#ifndef DEBUG_BUFFER_STRING_SIZE
#define DEBUG_BUFFER_STRING_SIZE 16384
#endif

// Should be constant accross all shaders (contrary to the shader string capacity)
#ifndef BUFFER_STRING_CAPACITY
// In Number of uint32_t
#define BUFFER_STRING_CAPACITY 64 
#endif

#define BUFFER_STRING_PACKED_CAPACITY (BUFFER_STRING_CAPACITY / 4)

struct BufferStringMeta
{
	vec2 position;
	uint layer;
	uint len;
	vec2 glyph_size;
	vec4 color;
	vec4 back_color;
	uint flags;
};

struct BufferString
{
	BufferStringMeta meta;
	// Store in uvec4 for 128 bit memory transactions?
	uint32_t data[BUFFER_STRING_PACKED_CAPACITY];
};

#define DEBUG_ENABLE_MASK	0x1
#define DEBUG_ENABLE_BIT	0x1

#define DEBUG_SPACE_MASK		(0b11 << 1)
#define DEBUG_PIXEL_SPACE_BIT	(0x0 << 1)
#define DEBUG_UV_SPACE_BIT		(0x1 << 1)
#define DEBUG_CLIP_SPACE_BIT	(0x2 << 1)

#ifndef BIND_DEBUG_BUFFERS
#define BIND_DEBUG_BUFFERS (I_WANT_TO_DEBUG & GLOBAL_ENABLE_GLSL_DEBUG)
#endif

#if BIND_DEBUG_BUFFERS

#ifndef DEBUG_BUFFER_BINDING
#define DEBUG_BUFFER_BINDING set = 0, binding = 0 
#endif

#if DEBUG_BUFFER_ACCESS_readonly
#define DEBUG_BUFFER_ACCESS readonly
#endif

#ifndef DEBUG_BUFFER_ACCESS
#define DEBUG_BUFFER_ACCESS
#endif

layout(DEBUG_BUFFER_BINDING) restrict DEBUG_BUFFER_ACCESS buffer DebugStringBuffer
{
	uint string_counter;
	BufferString strings[DEBUG_BUFFER_STRING_SIZE];
} _debug;

#endif

uint allocateDebugStrings(uint n)
{
#if BIND_DEBUG_BUFFERS && !DEBUG_BUFFER_ACCESS_readonly
	return atomicAdd(_debug.string_counter, n) % DEBUG_BUFFER_STRING_SIZE;
#endif
	return 0;
}

uint allocateDebugString()
{
	return allocateDebugStrings(1);
}

struct DebugStringCaret
{
	vec2 pos;
	uint layer;
};
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
	return 3.0f;
#endif
}

vec2 debugStringDefaultGlyphSizePix()
{
	return GLYPH_SIZE_PIX * getGlyphSizeMult();
}

vec4 debugStringDefaultFrontColor()
{
	return vec4(1);
}

vec4 debugStringDefaultBackColor()
{
	return vec4(0..xxx, 0.05);
}

Caret pushToDebugPix(const in ShaderString str, Caret c, bool ln)
{
#if BIND_DEBUG_BUFFERS && !DEBUG_BUFFER_ACCESS_readonly
	BufferStringMeta meta;
	meta.position = c.pos;
	meta.layer = c.layer;
	meta.len = getShaderStringLength(str);
	meta.glyph_size = debugStringDefaultGlyphSizePix();
	meta.color = debugStringDefaultFrontColor();
	meta.back_color = debugStringDefaultBackColor();
	meta.flags = DEBUG_PIXEL_SPACE_BIT;

	const uint index = allocateDebugString();

	_debug.strings[index].meta = meta;
	for(int i = 0; i < (meta.len + 3) / 4; ++i)
	{
		_debug.strings[index].data[i] = str.data[i];
	}
	
	Caret res = c;
	if(ln)
	{
		res.pos.y += meta.glyph_size.y;
	}
	else
	{
		res.pos.x += meta.glyph_size.x * meta.len;
	}
	return res;
#else
	return c;
#endif
}

