#pragma once

#ifndef GLOBAL_ENABLE_SHADER_DEBUG
#define GLOBAL_ENABLE_SHADER_DEBUG 1
#endif

#include "String.slang"
#include "DebugBuffersDefinitions.h"
#include <ShaderLib/span.slang>

#ifndef DEBUG_ENABLE_DEBUG_GLOBAL_SIGNAL
#define DEBUG_ENABLE_DEBUG_GLOBAL_SIGNAL 0
#endif 

#if DEBUG_ENABLE_DEBUG_GLOBAL_SIGNAL
static bool _g_debug_signal = false;
#endif

typealias DebugLowPFloat = fp16IFP;
__generic<int N>
typealias DebugLowPVector = Vector<DebugLowPFloat, N>;
typealias DebugLowPvec2 = DebugLowPVector<2>;
typealias DebugLowPvec4 = DebugLowPVector<4>;

// Is it better to pack the atomic counters in the same cache line, or separate them?
// The first part should rather be in a CBO (maybe part of a global options UBO)
struct DebugBuffersHeader
{
	uint max_string;
	uint max_chunks;
	uint max_lines;
	uint pad_0_0;

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

// sizeof: Matrix3x4f
struct DebugBufferStringMeta
{
	vec3 position;
	uint layer;

	DebugLowPvec4 color;
	DebugLowPvec4 back_color;
	
	DebugLowPvec2 glyph_size;
	uint len;
	uint flags;
	uint content_index;
};

// sizeof: Matrix2x4f
// TODO test it
struct ThinDebugBufferStringMeta
{
	uvec2 packed_position_and_layer; // xyz: position stored in normalized format, w: layer
	uint packed_len_flags; // 16 / 16
	DebugLowPvec2 glyph_size;

	// 6 colors + 2 alphas + 20+ bits for the content index
	// 128 bits
	// Color: UFloat15 : 6 * 15 = 90 bits
	// alpha: unorm8: 2 * 8 = 16 bits
	// index: 22 bits
	uvec4 packed_color_bg_content_index;
}

// sizeof: Matrix3x4f
struct DebugBufferLine
{
	vec4 p1;
	
	vec4 p2;
	
	DebugLowPvec4 color1;
	DebugLowPvec4 color2;
	
	uint layer;
	uint flags;
	uint pad1;
	uint pad2;
};
// TODO compress it to sizeof Matrix2x4f


#if BIND_DEBUG_BUFFERS

#ifndef DEBUG_BUFFER_ACCESS
#define DEBUG_BUFFER_ACCESS RWM
#endif

#define DebugStructuredBuffer BINDING_HANDLE(StructuredBuffer, DEBUG_BUFFER_ACCESS)
#define DebugRawBuffer BINDING_HANDLE(ByteAddressBuffer, DEBUG_BUFFER_ACCESS)

typealias DebugBuffersHeaderType = DebugStructuredBuffer<DebugBuffersHeader, Std430DataLayout>;
typealias DebugStringsMetasBufferType = DebugStructuredBuffer<DebugBufferStringMeta, Std430DataLayout>;
typealias DebugLinesBufferType = DebugStructuredBuffer<DebugBufferLine, Std430DataLayout>;
typealias DebugStringsContentBufferType = DebugRawBuffer;

layout(DEBUG_BUFFER_BINDING + 0) DebugBuffersHeaderType b_debug_buffers_header;
layout(DEBUG_BUFFER_BINDING + 1) DebugStringsMetasBufferType b_debug_buffers_strings_meta;
layout(DEBUG_BUFFER_BINDING + 2) DebugStringsContentBufferType b_debug_buffers_strings_content;
layout(DEBUG_BUFFER_BINDING + 3) DebugLinesBufferType b_debug_buffers_lines;

#if DEBUG_BUFFER_ACCESS == RWM
#define SHALLOW_DEBUGGING 0
#endif

#endif

#ifndef DEBUG_BUFFER_ACCESS
#define DEBUG_BUFFER_ACCESS ROM
#endif

#ifndef SHALLOW_DEBUGGING
#define SHALLOW_DEBUGGING 1
#endif

uint AllocateDebugStringMeta(uint n = 1)
{
	uint res = 0;
#if BIND_DEBUG_BUFFERS && DEBUG_BUFFER_ACCESS == RWM
	InterlockedAdd(b_debug_buffers_header[0].strings_counter, n, res);
#endif
	return res;
}

// len in chunks of u32
uint AllocateDebugStringContent(uint len)
{
	uint res = 0;
#if BIND_DEBUG_BUFFERS && DEBUG_BUFFER_ACCESS == RWM
	InterlockedAdd(b_debug_buffers_header[0].chunks_counter, len, res);
#endif
	return res;
}

uint AllocateDebugLine(uint n = 1)
{
	uint res = 0;
#if BIND_DEBUG_BUFFERS && DEBUG_BUFFER_ACCESS == RWM
	InterlockedAdd(b_debug_buffers_header[0].lines_counter, n, res);
#endif
	return res;
}

__generic <typename char_t : IChar, typename Chunk : IChunk<char_t> = DefaultChunk<char_t>, uint Compile_Time_Size = uint(-1)>
struct DebugBufferBasicStringSpan : IRWArray<Chunk>
{
	uint offset = 0;
	uint size = 0;

	uint getCount()
	{
		if(Compile_Time_Size == uint(-1))
		{
			return size;
		}
		else
		{
			return Compile_Time_Size;
		}
	}

	__subscript(uint i) -> Chunk
	{
		get {
			#if BIND_DEBUG_BUFFERS
				return b_debug_buffers_strings_content.LoadAligned<Chunk>(i * sizeof(Chunk) + offset);
			#else
				return Chunk();
			#endif
		}
		set {
			#if BIND_DEBUG_BUFFERS && (DEBUG_BUFFER_ACCESS == RWM)
				b_debug_buffers_strings_content.StoreAligned(i * sizeof(Chunk) + offset, newValue);
			#endif
		}
	}
};

// __generic<typename char_t : IChar, typename Chunk : IChunk<char_t>>
// extension DebugBufferBasicStringSpan<char_t, Chunk, uint(-1)>
// {
// 	uint size = 0;
	
// 	[OverloadRank(15)]
// 	uint getCount()
// 	{
// 		return size;
// 	}

// 	[mutating]
// 	uint setCount(uint new_size)
// 	{
// 		size = new_size;
// 	}
// };

__generic <typename char_t : IChar, typename Chunk : IChunk<char_t> = DefaultChunk<char_t>>
typealias DebugBufferBasicString = BasicStringBase<
	char_t, 
	Chunk, 
	DebugBufferBasicStringSpan<char_t, Chunk>
>;

typealias DebugBufferString = DebugBufferBasicString<char, DefaultChunk<char>>;

__generic <typename char_t : IChar = char>
DebugBufferBasicString<char_t, DefaultChunk<char_t>> MakeEmptyDebugBufferBasicString(uint chunk_index, uint capacity)
{
	DebugBufferBasicString<char_t, DefaultChunk<char_t>> res;
	res._chunks.size = capacity;
	res._chunks.offset = chunk_index * (sizeof(DefaultChunk<char_t>));
	res._size = 0;
	return res;
}

DebugBufferString MakeEmptyDebugBufferString(uint index, uint capacity)
{
	return MakeEmptyDebugBufferBasicString<char>(index, capacity);
}

__generic <typename char_t : IChar = char>
DebugBufferBasicString<char_t, DefaultChunk<char_t>> MakeEmptyDebugBufferBasicString(uint capacity)
{
	const uint chunk_index = AllocateDebugStringContent(capacity);
	return MakeEmptyDebugBufferBasicString<char_t>(chunk_index, capacity);
}

DebugBufferString MakeEmptyDebugBufferString(uint capacity)
{
	return MakeEmptyDebugBufferBasicString<char>(capacity);
}


[Flags]
enum Print
{
	Ln = 0x0,
	SameLine = 0x1,
};

static const Print DefaultPrintFlags = Print::Ln;

struct DebugPrinter
{
	struct State
	{
		DebugLowPvec4 color = DebugLowPvec4(1, 1, 1, 1);
		DebugLowPvec4 back_color = DebugLowPvec4(0, 0, 0, 0.5);
		DebugLowPvec2 glyph_size; 
		uint flags = 0;

		static float GetFontSize(int font_size)
		{
			float res = exp2(float(font_size - FONT_SIZE_NORMAL));
			return res;
		}

		[mutating]
		void setDefaultGlyphSize(int font_size = DEFAULT_FONT_SIZE)
		{
			const vec2 oo_screen_size = rcp(vec2(1600, 900));
			const float scale = GetFontSize(font_size);
			vec2 pix_size = GLYPH_SIZE_PIX * scale;
			this.glyph_size = DebugLowPvec2(pix_size);
			if((this.flags & DEBUG_PIXEL_SPACE_BIT) != 0)	this.glyph_size;
			if((this.flags & DEBUG_UV_SPACE_BIT) != 0)		this.glyph_size *= DebugLowPvec2(oo_screen_size);
			if((this.flags & DEBUG_CLIP_SPACE_BIT) != 0)	this.glyph_size *= DebugLowPvec2(oo_screen_size * 2.0f);
		}

		__init(int flags = 0, int font_size = DEFAULT_FONT_SIZE)
		{
			this.flags = flags;
			setDefaultGlyphSize(font_size);
		}
	};

	State state;

	vec3 position;
	uint layer = 0;
	float offset = 0;

	__init()
	{
		state = State();
		position = vec3(0);
	}

	[mutating]
	void updateCaret(DebugLowPvec2 glyph_size, uint len, uint flags, Print print_flags)
	{
		const bool crlf = (flags & Print::SameLine) == 0;
		const float len_f = float(len);
		const bool horizontal = true;
		if(crlf)
		{
			offset = 0;
			if(horizontal)	position.xy += vec2(0, glyph_size.y);
			else			position.xy += vec2(glyph_size.x, 0);	
		}
		else
		{
			if(horizontal)	offset += glyph_size.x * len_f;
			else			offset += glyph_size.y * len_f;
		}
	}

	void commitMeta(uint meta_index, uint string_length, uint content_index)
	{
#if !SHALLOW_DEBUGGING
		DebugBufferStringMeta meta;
		const bool horizontal = true;
		meta.position = this.position;
		if(horizontal)	meta.position.x += offset;
		else			meta.position.y += offset;
		meta.color = state.color;
		meta.back_color = state.back_color;
		meta.flags = state.flags;
		meta.glyph_size = state.glyph_size;
		meta.len = string_length;
		meta.content_index = content_index;
		b_debug_buffers_strings_meta[meta_index] = meta;
#endif
	}

	static void HelperAccumulator(inout uint dest, const in IPrintable p)
	{
		dest += p.getPrintableImpl().estimateStringSize();
	}

	[mutating]
	__generic<each P>
	void print(Print print_flags, expand each P args)
		where P : IPrintable
	{
#if !SHALLOW_DEBUGGING
		const uint str_index = AllocateDebugStringMeta();
		uint required_length = 0;
		expand HelperAccumulator(required_length, each args);
		const uint capacity = DivUpAssumePo2(required_length, DefaultChunk<char>::Capacity());
		const uint content_index = AllocateDebugStringContent(capacity);
		var str = MakeEmptyDebugBufferString(content_index, capacity);
		expand str.append(each args);

		commitMeta(str_index, str.size(), content_index);
		updateCaret(state.glyph_size, str.size(), state.flags, print_flags);
#endif
	}

	[mutating]
	__generic<each P : IPrintable>
	void print(const in expand each P args)
	{
		print(DefaultPrintFlags, args);
	}

	static DebugLowPvec4 GetDimColor(uint d)
	{
		DebugLowPvec4 res;
		res.w = DebugLowPFloat(1);
		if(d < 3)
		{
			res.xyz = DebugLowPFloat(0);
			res[d] = DebugLowPFloat(1);
		}
		else if(d == 4)
		{
			res.x = DebugLowPFloat(1);
			res.z = DebugLowPFloat(1);
			res.y = DebugLowPFloat(0);
		}
		else
		{
			res.xyz = DebugLowPFloat(1);
		}
		return res;
	} 

	[mutating]
	__generic<P : IPrintable, int N>
	void printVec(const in Vector<P, N> v, Print print_flags = DefaultPrintFlags)
	{
		DebugLowPvec4 push_color = state.color;
		for(uint i = 0; i < N; ++i)
		{
			state.color = GetDimColor(i);
			print(print_flags, v[i]);
		}
		state.color = push_color;
	}

	[mutating]
	__generic<P : IPrintable, int R, int C>
	void printMat(const in Matrix<P, R, C> m, Print print_flags = DefaultPrintFlags)
	{
		DebugLowPvec4 push_color = state.color;
		for(uint i = 0; i < R; ++i)
		{
			state.color = GetDimColor(i);
			Vector<P, C> row = GetRow(m, i);
			print(print_flags, row);
		}
		state.color = push_color;
	}
};








#ifndef DEBUG_ENABLE_GLOBAL_PRINTER
#define DEBUG_ENABLE_GLOBAL_PRINTER 0
#endif

#if DEBUG_ENABLE_GLOBAL_PRINTER
static DebugPrinter _g_debug_printer;
#endif


void StoreDebugLine(const in DebugBufferLine line, uint index)
{
#if BIND_DEBUG_BUFFERS && DEBUG_BUFFER_ACCESS == RWM
	b_debug_buffers_lines[index] = line;
#endif
}

[Flags]
enum DebugLine
{
	ClipSpace = DEBUG_CLIP_SPACE_BIT,
	UVSpace = DEBUG_UV_SPACE_BIT,
	PixelSpace = DEBUG_PIXEL_SPACE_BIT,
};

void PushDebugLine(const in DebugBufferLine line)
{
	uint index = AllocateDebugLine();
	StoreDebugLine(line, index);
}

void PushDebugLine(vec4 p1, vec4 p2, uint layer, DebugLowPvec4 c1, DebugLowPvec4 c2, DebugLine flags)
{
	DebugBufferLine line;

	line.p1 = p1;
	line.p2 = p2;
	line.color1 = c1;
	line.color2 = c2;
	line.layer = layer;
	line.flags = uint(flags) | DEBUG_ENABLE_BIT;

	PushDebugLine(line);
}


void PushDebugLine(Matrix4f xform, vec3 p1, vec3 p2, uint layer, DebugLowPvec4 c1, DebugLowPvec4 c2, DebugLine flags)
{
	PushDebugLine(xform * vec4(p1, 1), xform * vec4(p2, 1), layer, c1, c2, flags | DebugLine::ClipSpace);
}

void PushDebugLine(Matrix4f xform, vec3 p1, vec3 p2, uint layer, DebugLowPvec4 color, DebugLine flags)
{
	PushDebugLine(xform, p1, p2, layer, color, color, flags);
}

void PushDebugLine(vec2 p1, vec2 p2, uint layer, DebugLowPvec4 c1, DebugLowPvec4 c2, DebugLine flags = DebugLine::UVSpace)
{
	PushDebugLine(vec4(p1, 0, 1), vec4(p2, 0, 1), layer, c1, c2, flags);
}

void PushDebugLine(vec2 p1, vec2 p2, uint layer, DebugLowPvec4 color, DebugLine flags = DebugLine::UVSpace)
{
	PushDebugLine(p1, p2, layer, color, color, flags);
}



