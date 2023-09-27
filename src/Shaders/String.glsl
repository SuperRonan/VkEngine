#pragma once

#include "common.glsl"
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

#ifndef HEX_USE_CAPS
#define HEX_USE_CAPS 0
#endif

#ifndef DEFAULT_NUMBER_BASIS
#define DEFAULT_NUMBER_BASIS 10u
#endif

#ifndef DEFAULT_SHOW_PLUS
#define DEFAULT_SHOW_PLUS false
#endif

#ifndef DEFAULT_FLOAT_PRECISION
#define DEFAULT_FLOAT_PRECISION 3u
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
	ShaderString res;
	for(uint c = 0; c < SHADER_STRING_PACKED_CAPACITY; ++c)
	{
		res.data[c] = uint32_t(0);
	}
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
	// Read from the begining of the string
	// The chunk read will be done in pendingRead
	return PendingChunk(0, 0);
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

PendingChunk getPendingChunkForWritingReverse(const in ShaderString s)
{
#if ENABLE_SHADER_STRING
	const uint ls = getShaderStringLength(s);	
	const uint32_t c = s.data[(ls-1) / 4];
	return PendingChunk(ls-1, c);
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
	const char res = char((chunk.chunk >> (8 * id_in_chunk)) & 0xff); 
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
		chunk.chunk = 0;
	}
	++chunk.index;
#endif
}

void pendingWriteReverse(inout PendingChunk chunk, inout ShaderString str, char c)
{
#if ENABLE_SHADER_STRING
	const uint id_in_chunk = chunk.index % 4;
	chunk.chunk |= (uint32_t(c) << (id_in_chunk * 8));
	if(id_in_chunk == 0)
	{
		str.data[chunk.index / 4] = chunk.chunk;
		chunk.chunk = str.data[chunk.index / 4 - 1];
	}
	--chunk.index;
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

void flushPendingWriteReverseIFN(const in PendingChunk chunk, inout ShaderString str)
{
#if ENABLE_SHADER_STRING
	//if((chunk.index % 4) != 3)
	{
		str.data[chunk.index / 4] = chunk.chunk;
	}
#endif
}

void appendOneChar(inout ShaderString s, char c)
{
#if ENABLE_SHADER_STRING
	const uint ls = getShaderStringLength(s);
	assert(ls + 1 < SHADER_STRING_CAPACITY);
	setShaderStringChar(s, ls, c);
	setShaderStringLength(s, ls + 1);
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

#define CHAR_space 		char(0x20)
#define CHAR_lpar 		char(0x28)
#define CHAR_rpar 		char(0x29)
#define CHAR_plus 		char(0x2B)
#define CHAR_comma		char(0x2C)
#define CHAR_minus 		char(0x2D)
#define CHAR_dot 		char(0x2E)
#define CHAR_0 			char(0x30)
#define CHAR_A 			char(0x41)
#define CHAR_at 		char(0x40)
#define CHAR_a 			char(0x61)

uint howManyDigits(uint n, uint basis)
{
	if(n == 0)	return 1;
	uint res = 0;
	while(n != 0)
	{
		n /= basis;
		++ res;
	}
	return res;
}

char getASCII(uint n)
{
	if(n < 10)	return char(CHAR_0 + n);
#if HEX_USE_CAPS
	else 		return char(CHAR_A + (n - 10));
#else
	else 		return char(CHAR_a + (n - 10));
#endif
}

ShaderString getBasisPrefix(uint b)
{
	if(b == 2)			return "0b";
	else if(b == 8)		return "0";
	else if(b == 16)	return "0x";
	else 				return "";
}

void append(inout ShaderString s, uint n, uint basis, uint len)
{
#if ENABLE_SHADER_STRING
	const uint ls = getShaderStringLength(s);
	setShaderStringLength(s, ls + len);
	PendingChunk writer = getPendingChunkForWritingReverse(s);
	
	for(uint i = 0; i < len; ++i)
	{
		const uint digit = n % basis;
		n /= basis;
		char ascii = getASCII(digit);
		pendingWriteReverse(writer, s, ascii);
	}
	flushPendingWriteReverseIFN(writer, s);
#endif
}

void append(inout ShaderString s, uint n, uint basis)
{
	const uint len = howManyDigits(n, basis);
	append(s, n, basis, len);
}

ShaderString concat(ShaderString s, uint n, uint b)
{
	append(s, n, b);
	return s;
}

ShaderString concat(ShaderString s, uint n)
{
	return concat(s, n, DEFAULT_NUMBER_BASIS);
}

ShaderString toStr(uint n, uint basis)
{
	ShaderString res = makeShaderString();
	append(res, n, basis);
	return res;
}

ShaderString toStr(uint n)
{
	return toStr(n, DEFAULT_NUMBER_BASIS);
}

ShaderString concat(uint n, in const ShaderString s)
{
	ShaderString res = toStr(n);
	append(res, s);
	return res;
}

void append(inout ShaderString s, int n, uint basis, bool show_plus)
{
	const uint a = abs(n);
	const bool neg = n < 0;
	if(neg)
	{
		appendOneChar(s, CHAR_minus);
	}
	else if(show_plus)
	{
		appendOneChar(s, CHAR_plus);
	}
	append(s, a, basis);
}

void append(inout ShaderString s, int n)
{
	append(s, n, DEFAULT_NUMBER_BASIS, DEFAULT_SHOW_PLUS);
}

ShaderString toStr(int n, uint basis, bool show_plus)
{
	ShaderString res = makeShaderString();
	append(res, n, basis, show_plus);
	return res;
}

ShaderString toStr(int n)
{
	return toStr(n, DEFAULT_NUMBER_BASIS, DEFAULT_SHOW_PLUS);
}

ShaderString concat(ShaderString s, int n)
{
	append(s, n);
	return s;
}

ShaderString concat(int n, in const ShaderString s)
{
	ShaderString res = toStr(n);
	append(res, s);
	return res;
}

void append(inout ShaderString s, float f, uint flt_precision, bool show_plus)
{
	// Never gonna show float in hex or bin
	const uint basis = 10;
	const bool neg = f < 0.0f;
	if(isnan(f))
	{
		append(s, "nan");
		return;
	}
	if(neg)
	{
		appendOneChar(s, CHAR_minus);
	}
	else if(show_plus)
	{
		appendOneChar(s, CHAR_plus);
	}
	
	if(isinf(f))
	{
		append(s, "inf");
		return;
	}


	const uint integral_part = uint(abs(f));
	const float dec = abs(f - trunc(f));
	uint dec_part = uint(dec * pow(basis, flt_precision));
	
	append(s, integral_part, basis);
	appendOneChar(s, CHAR_dot);
	append(s, dec_part, basis, flt_precision);
}

void append(inout ShaderString s, float f, uint flt_precision)
{
	append(s, f, flt_precision, DEFAULT_SHOW_PLUS);
}

void append(inout ShaderString s, float f)
{
	append(s, f, DEFAULT_FLOAT_PRECISION);
}

ShaderString toStr(float f, uint flt_precision, bool show_plus)
{
	ShaderString res = makeShaderString();
	append(res, f, flt_precision, show_plus);
	return res;
}

ShaderString toStr(float f, uint flt_precision)
{
	return toStr(f, flt_precision, DEFAULT_SHOW_PLUS);
}

ShaderString toStr(float f)
{
	return toStr(f, DEFAULT_FLOAT_PRECISION);
}

ShaderString concat(ShaderString s, float f, uint flt_precision)
{
	append(s, f, flt_precision);
	return s;
}

ShaderString concat(ShaderString s, float f)
{
	append(s, f);
	return s;
}

ShaderString concat(float f, in const ShaderString s)
{
	ShaderString res = toStr(f);
	append(res, s);
	return res;
}

// Wish I had templates

#define DECLARE_append_gvec_impl(vecType, N) void append(inout ShaderString s, in const vecType##N v) { appendOneChar(s, CHAR_lpar); for(uint i=0; i < N; ++i)	{ append(s, v[i]); if(i != (N - 1))	{appendOneChar(s, CHAR_comma); appendOneChar(s, CHAR_space); } } appendOneChar(s, CHAR_rpar); }

#define DECLARE_append_gvec_for_type(vecType) DECLARE_append_gvec_impl(vecType, 2) DECLARE_append_gvec_impl(vecType, 3) DECLARE_append_gvec_impl(vecType, 4)

#define DECLARE_append_gvec DECLARE_append_gvec_for_type(vec) DECLARE_append_gvec_for_type(uvec) DECLARE_append_gvec_for_type(ivec)

#define DECLARE_toStr_gvec_impl(vecType, N) ShaderString toStr(in const vecType##N v) { ShaderString res = makeShaderString(); append(res, v); return res; }

#define DECLARE_toStr_gvec_for_type(vecType) DECLARE_toStr_gvec_impl(vecType, 2) DECLARE_toStr_gvec_impl(vecType, 3) DECLARE_toStr_gvec_impl(vecType, 4)

#define DECLARE_toStr_gvec DECLARE_toStr_gvec_for_type(vec) DECLARE_toStr_gvec_for_type(uvec) DECLARE_toStr_gvec_for_type(ivec)

#define DECLARE_concat_gvec_impl(vecType, N) ShaderString concat(ShaderString s, in const vecType##N vec) { append(s, vec); return s; } ShaderString concat(in const vecType##N vec, in const ShaderString s) {	ShaderString res = toStr(vec); append(res, s); return res; }

#define DECLARE_concat_gvec_for_type(vecType) DECLARE_concat_gvec_impl(vecType, 2) DECLARE_concat_gvec_impl(vecType, 3) DECLARE_concat_gvec_impl(vecType, 4)

#define DECLARE_concat_gvec DECLARE_concat_gvec_for_type(vec) DECLARE_concat_gvec_for_type(uvec) DECLARE_concat_gvec_for_type(ivec)

DECLARE_append_gvec
DECLARE_toStr_gvec
DECLARE_concat_gvec