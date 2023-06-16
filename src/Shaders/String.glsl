#pragma once

#include "interop_cpp.glsl"

#ifndef I_WANT_TO_DEBUG
#define I_WANT_TO_DEBUG 0
#endif

#ifndef GLOBAL_ENABLE_GLSL_DEBUG
#define GLOBAL_ENABLE_GLSL_DEBUG 0
#endif


#ifndef SHADER_STRING_CAPACITY
#define SHADER_STRING_CAPACITY 32
#endif

#if ((SHADER_STRING_CAPACITY % 4) != 0)
#error "SHADER_STRING_CAPACITY must be a multiple of 4"
#endif

#define SHADER_STRING_PACKED_CAPACITY (SHADER_STRING_CAPACITY / 4)

#ifndef ENABLE_SHADER_STRING
#define ENABLE_SHADER_STRING (GLOBAL_ENABLE_GLSL_DEBUG && I_WANT_TO_DEBUG)
#endif

#extension GL_EXT_shader_explicit_arithmetic_types : require

#define char uint8_t

struct ShaderString
{
#if ENABLE_SHADER_STRING
	// Last char is the size
	uint32_t data[SHADER_STRING_PACKED_CAPACITY];
	#ifdef __cplusplus
		ShaderString(const in uint32_t data[SHADER_STRING_PACKED_CAPACITY]);
	#endif
#else
	int pad;
#endif
};

#define Str ShaderString

ShaderString makeShaderString(const in uint32_t data[SHADER_STRING_PACKED_CAPACITY])
{
#if ENABLE_SHADER_STRING
	ShaderString res = ShaderString(data);
#else
	ShaderString res = ShaderString(0);
#endif
	return res;
}

ShaderString makeShaderString()
{
#if ENABLE_SHADER_STRING
	uint32_t data[SHADER_STRING_PACKED_CAPACITY];
	for(uint c = 0; c < SHADER_STRING_PACKED_CAPACITY; ++c)
	{
		data[c] = uint32_t(0);
	}
	ShaderString res = ShaderString(data);
#else
	ShaderString res = ShaderString(0);
#endif
	return res;
}

char getShaderStringChar(const in ShaderString s, uint i)
{
#if ENABLE_SHADER_STRING
	const uint chunk_index = (i / 4);
	const uint char_in_chunk = (i % 4);
	const uint32_t chunk = s.data[chunk_index];
	const uint c = (chunk >> (char_in_chunk * 8)) & 0xff;
	return char(c);
#else
	return char(0);
#endif
}

char getCharInChunk(uint c, uint i)
{
	return char((c >> (i * 8)) & 0xff);
}

uint32_t charMask(uint i)
{
	return 0xff << (i * 8);
}

uint32_t placeChar(uint i, char c)
{
	return uint32_t(c) << (i * 8);
}

void setCharInChunk(inout uint32_t chunk, uint i, char c)
{
	chunk = (chunk & ~charMask(i)) | placeChar(i, c);
}

void setShaderStringChar(inout ShaderString s, uint i, char c)
{
#if ENABLE_SHADER_STRING
	const uint chunk_index = (i / 4);
	const uint char_in_chunk = (i % 4);
	uint32_t chunk = s.data[chunk_index];
	setCharInChunk(chunk, char_in_chunk, c);
	s.data[chunk_index] = chunk;
#endif
}

uint getShaderStringLength(const in ShaderString s)
{
#if ENABLE_SHADER_STRING
	uint32_t chunk = s.data[SHADER_STRING_PACKED_CAPACITY - 1];
	uint res = (chunk >> (3 * 8));
	return res;
#else
	return 0;
#endif
}

void setShaderStringLength(inout ShaderString s, uint l)
{
#if ENABLE_SHADER_STRING
	uint32_t chunk = s.data[SHADER_STRING_PACKED_CAPACITY - 1];
	setCharInChunk(chunk, SHADER_STRING_CAPACITY-1, char(l));
	s.data[SHADER_STRING_PACKED_CAPACITY - 1] = chunk;
#endif
}

struct PendingChunk
{
	uint index;
	uint32_t chunk;
#ifdef __cplusplus
	PendingChunk(uint index = 0, uint32_t chunk = 0) : 
		index(index),
		chunk(chunk)
	{}
#endif
};

PendingChunk getPendingChunkForReading(const in ShaderString s)
{
#if ENABLE_SHADER_STRING
	const uint ls = getShaderStringLength(s);	
	const uint32_t c = (ls % 4 != 0) ? s.data[ls / 4] : 0;
	return PendingChunk(ls, c);
#else
	return PendingChunk(0, 0);
#endif
}

PendingChunk getPendingChunkForWriting(const in ShaderString s)
{
#if ENABLE_SHADER_STRING
	const uint ls = getShaderStringLength(s);	
	const uint32_t c = s.data[ls / 4];
	return PendingChunk(ls, c);
#else
	return PendingChunk(0, 0);
#endif
}

char pendingRead(inout PendingChunk chunk, const in ShaderString str)
{
#if ENABLE_SHADER_STRING
	const uint id_in_chunk = chunk.index % 4;
	if(id_in_chunk == 0)
	{
		chunk.chunk = str.data[chunk.index / 4];
	}
	char res = char((chunk.chunk >> (8 * id_in_chunk)) & 0xff); 
	++chunk.index;
	return res;
#else
	return char(0);
#endif
}

void pendingWrite(inout PendingChunk chunk, inout ShaderString str, char c)
{
#if ENABLE_SHADER_STRING
	const uint id_in_chunk = chunk.index % 4;
	chunk.chunk |= (uint32_t(c) << (id_in_chunk * 8));
	if(id_in_chunk == 3)
	{
		str.data[chunk.index / 4] = chunk.chunk;
	}
	++chunk.index;
#endif
}

void flushPendingWriteIFN(const in PendingChunk chunk, inout ShaderString str)
{
#if ENABLE_SHADER_STRING
	if((chunk.index % 4) != 0)
	{
		str.data[chunk.index / 4] = chunk.chunk;
	}
#endif
}

void append(inout ShaderString s, const in ShaderString t)
{
#if ENABLE_SHADER_STRING
	const uint ls = getShaderStringLength(s);
	const uint lt = getShaderStringLength(t);
	assert(ls + lt < SHADER_STRING_CAPACITY);
	
	PendingChunk writer = getPendingChunkForWriting(s);
	PendingChunk reader = getPendingChunkForReading(t);

	for(uint i = 0; i < lt; ++i)
	{
		const char c = pendingRead(reader, t);
		pendingWrite(writer, s, c);
	}
	flushPendingWriteIFN(writer, s);
	setShaderStringLength(s, ls + lt);
#endif
}

ShaderString concat(ShaderString a, const in ShaderString b)
{
	append(a, b);
	return a;
}

#define CHAR_SPACE char(0x20)
#define CHAR_0 char(0x30)
#define CHAR_minus char(0x2D)
#define CHAR_dot char(0x2E)
#define CHAR_A char(0x41)
#define CHAR_at char(0x40)
#define CHAR_a char(0x61)

void appendDec(inout ShaderString s, uint n)
{
	PendingChunk writer = getPendingChunkForWriting(s);
	//while(n != 0)
	{
		
	}
}