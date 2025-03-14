#pragma once

#include <ShaderLib/common.slang>

#include "StringDefinitions.h"

#include <ShaderLib/Bit.slang>

#ifndef GLOBAL_ENABLE_SLANG_DEBUG
#define GLOBAL_ENABLE_SLANG_DEBUG GLOBAL_ENABLE_SHADER_DEBUG
#endif

typealias char = uint8_t;

// template <typename Chunk, bool Reverse = false>
// struct PendingChunk
// {
// 	Chunk chunk;
// 	uint index;
// };

__generic <typename UInt : __BuiltinIntegerType>
uint HowManyDigitsAnyBasis(UInt n, UInt base)
{
	uint res = 0;
	while(n != UInt(0))
	{
		n /= base;
		++res;
	}
	return max(res, 1);
}

__generic<typename UInt : __BuiltinIntegerType>
uint HowManyDigitsAssumePo2Basis(UInt n, uint basis)
{
	uint w = BitWidth(n);
	uint d = (FirstBitHigh(basis));
	return max(DivUpSafe(w, d), 1);
}

__generic <typename UInt : __BuiltinIntegerType>
uint HowManyDigits(UInt n, uint basis)
{
	uint res;
	if(IsPo2(basis))
		res = HowManyDigitsAssumePo2Basis(n, basis);
	else
		res = HowManyDigitsAnyBasis(n, UInt(basis));
	return res;
}

struct IntegerFormat
{
	u8 basis;
	i8 digits; // `0`: as many digits as necessary, `-1`: as many digits as needed to represent the max value in basis 
	bool show_plus;
	bool add_basis_prefix;

	__init()
	{
		basis = u8(DEFAULT_NUMBER_BASIS);
		digits = i8(0); 
		show_plus = DEFAULT_SHOW_PLUS;
		add_basis_prefix = this.basis != u8(10);
	}

	__generic<UInt : __BuiltinIntegerType>
	uint MaxDigitsForBasis()
	{
		const uint bits_per_digit = FirstBitHigh((uint(basis))); // Same as FirstBitHigh(BitFloor(basis))
		const uint bits = 8 * sizeof(UInt);
		uint res = DivUpSafe(bits, bits_per_digit);
		return res;
	}

	__generic<UInt : __BuiltinIntegerType>
	uint HowManyDigits(UInt value)
	{
		uint res;
		if(digits == 0)
		{
			res = ::HowManyDigits(value, uint(basis));
		}
		else if(digits < 0)
		{
			res = MaxDigitsForBasis<UInt>();
		}
		else
		{
			res = digits;
		}
		return res;
	}

	__generic<UInt : __BuiltinIntegerType>
	uint HowManyDigitsFastConservative(UInt value)
	{
		uint res;
		if(digits == 0)
		{
			res = HowManyDigitsAssumePo2Basis(value, BitFloor(uint(basis)));
		}
		else if(digits < 0)
		{
			res = MaxDigitsForBasis<UInt>();
		}
		else
		{
			res = digits;
		}
		return res;
	}
};

struct FloatFormat
{
	u16 precision;
	bool show_plus;

	__init()
	{
		precision = u16(DEFAULT_FLOAT_PRECISION);
		show_plus = DEFAULT_SHOW_PLUS;
	}
};



#define IChar __BuiltinIntegerType

__generic <char_t : IChar>
interface IChunk
{
	// in chars
	static constexpr uint Capacity();

	__init();

	//static_assert(Capacity() >= sizeof(char_t), "A chunk must be able to hold at least one char.");

	[mutating]
	void clear();

	char_t get(uint index);

	[mutating]
	void set(uint index, char_t char);

	[mutating]
	void setAssumeClear(uint index, char_t char);
};

__generic<char_t : IChar, UInt : __BuiltinIntegerType>
struct UIntChunk : IChunk<char_t>
{
	UInt u = UInt(0);

	static constexpr UInt CharMask(uint i = 0)
	{
		UInt res = BitMask<UInt>(UInt(sizeof(char_t) * 8));
		res = res << (8 * i);
		return res;
	}

	__init()
	{
		clear();
	}

	static constexpr uint Capacity()
	{
		const uint res = sizeof(UInt) / sizeof(char_t);
		static_assert(res >= 1, "A chunk must be able to hold at least one char.");
		return res;
	}

	[mutating]
	void clear()
	{
		u = UInt(0);
	}

	char_t get(uint index)
	{
		const UInt masked = (u >> (sizeof(char_t) * 8 * index)) & CharMask();
		return reinterpret<char_t>(masked);
	}

	[mutating]
	void setAssumeClear(uint index, char_t char)
	{
		UInt v = reinterpret<UInt>(char) << (sizeof(char_t) * 8 * index);
		u = u | v;
	}

	[mutating]
	void set(uint index, char_t char)
	{
		u = u & ~CharMask(index);
		setAssumeClear(index, char);
	}
	
}

__generic <typename char_t, bool Reverse = false>
interface IBasicStringCaret
{
	uint getIndex();

	char_t getChar(uint index_in_chunk = 0);
};

interface IGenericString1
{
	associatedtype CharType : IChar;

	uint capacity();
	
	uint size();
	uint getCount();

	[mutating]
	__generic <OtherString : IGenericString1>
	void appendString(const in OtherString other);

	[mutating]
	void appendArray(const in IArray<CharType> array);

	[mutating]
	void appendChar(CharType c);

	[mutating]
	__generic <UInt : __BuiltinUnsignedIntegerType>
	void appendUnsignedInteger(UInt n, const in IntegerFormat format);

	[mutating]
	__generic <Int : __BuiltinIntegerType>
	void appendInteger(Int n, const in IntegerFormat format);
	
	[mutating]
	__generic <Float : __BuiltinFloatingPointType>
	void appendFloat(Float f, const in FloatFormat format);
};

interface IPrintableImpl
{
	uint estimateStringSize();

	__generic <String : IGenericString1>
	void appendToString(inout String str);
};



__generic <typename T>
interface IPrintableImplValue : IPrintableImpl
{
	associatedtype PrintFormatType;

	
};

interface IPrintable
{
	associatedtype PrintableImpl : IPrintableImpl;

	PrintableImpl getPrintableImpl();
};

__generic <P : IPrintableImpl>
extension P : IPrintable
{
	typealias PrintableImpl = P;
	PrintableImpl getPrintableImpl()
	{
		return this;
	}
};

interface IGenericString2 : IGenericString1
{
	[mutating]
	void append(const in IPrintable printable);
};

__generic <typename char_t : IChar>
interface IBasicString : IGenericString2, IRWArray<char_t>, IPrintableImpl
{
	__subscript(uint i) -> char_t;

	IBasicStringCaret<char_t> getReader();
};

__generic <
	typename char_t : IChar, 
	typename Chunk : IChunk<char_t>, 
	typename Storage : IRWArray<Chunk>
>
struct BasicStringBase : IBasicString<char_t>
{
	typealias CharType = char_t;

	// chars are manually packed as uint because the optimizer tends to store each individual char in a uint
	Storage _chunks;
	uint _size = 0;

	constexpr uint size()
	{
		return _size;
	}

	[mutating]
	void setSize(uint s)
	{
		_size = s;
	}

	static constexpr uint CharsPerChunk()
	{
		return Chunk::Capacity();
	}
	
	constexpr uint capacity()
	{
		return _chunks.getCount() * CharsPerChunk();
	}

	constexpr uint getCount()
	{
		return size();
	}

	constexpr uint remainingCapacity()
	{
		return capacity() - size();
	}

	constexpr uint activeChunks()
	{
		return (size() + CharsPerChunk() - 1) / CharsPerChunk();
	}

	constexpr char_t getChar(uint i)
	{
		const uint chunk_index = i / CharsPerChunk(); 
		const uint index_in_chunk = i % CharsPerChunk(); 
		const Chunk chunk = _chunks[chunk_index];
		return chunk.get(index_in_chunk);
	}
	
	constexpr __subscript(uint i) -> char_t
	{
		get {return getChar(i);}
		set {
			// Can't call a mutating member from here :(
			const uint chunk_index = i / CharsPerChunk(); 
			const uint index_in_chunk = i % CharsPerChunk(); 
			_chunks[chunk_index].set(index_in_chunk, newValue);
		}
	}
	
	[mutating]
	void setChar(uint i, char_t v)
	{
		this[i] = v;
	}


	struct PendingChunkImpl<let Reverse : bool> : IBasicStringCaret<char_t, Reverse>
	{
		uint index = 0;
		Chunk chunk = {};

		uint getIndex()
		{
			return index;
		}

		char_t getChar(uint index_in_chunk)
		{
			return chunk.get(index_in_chunk);
		}
	};

	typealias PendingChunk = PendingChunkImpl<false>;
	typealias ReversePendingChunk = PendingChunkImpl<true>;

	PendingChunk getReader()
	{
		PendingChunk res = {};
		return res;
	}

	PendingChunk getWriter(uint size)
	{
		PendingChunk res;
		res.index = size;
		if(_size > 0)
		{
			res.chunk = _chunks[res.index / CharsPerChunk()];
		}
		return res;
	}

	ReversePendingChunk getReverseWriter(uint size)
	{
		return bit_cast<ReversePendingChunk>(getWriter(size - 1));
	}

	char_t read(inout PendingChunk reader)
	{
		const uint id_in_chunk = reader.index % CharsPerChunk();
		if(id_in_chunk == 0)
		{
			const uint chunk_id = reader.index / CharsPerChunk();
			reader.chunk = _chunks[chunk_id];
		}
		const char_t res = reader.chunk.get(id_in_chunk);
		++reader.index;
		return res;
	}

	[mutating]
	void write<let Reverse : bool>(inout PendingChunkImpl<Reverse> writer, char_t c)
	{
		const uint id_in_chunk = writer.index % CharsPerChunk();
		const uint chunk_id = writer.index / CharsPerChunk();
		writer.chunk.set(id_in_chunk, c);
		if(!Reverse)
		{
			if(id_in_chunk == (CharsPerChunk() - 1))
			{
				_chunks[chunk_id] = writer.chunk;
				writer.chunk.clear();
			}
			++writer.index;
		}
		else
		{
			if(id_in_chunk == (0))
			{
				_chunks[chunk_id] = writer.chunk;
				if(chunk_id > 0)
				{
					writer.chunk = _chunks[chunk_id-1];
				}
			}
			--writer.index;
		}
	}

	[mutating]
	void flushIFN<let Reverse : bool>(inout PendingChunkImpl<Reverse> writer)
	{
		const uint id_in_chunk = writer.index % CharsPerChunk();
		const uint chunk_id = writer.index / CharsPerChunk();
		if(!Reverse)
		{
			if(id_in_chunk != 0)
			{
				_chunks[chunk_id] = writer.chunk;
			}
		}
		else
		{
			_chunks[chunk_id] = writer.chunk;
		}
	}

	PendingChunk getWriter()
	{
		return getWriter(size());
	}

	ReversePendingChunk getReverseWriter()
	{
		return getReverseWriter(size());
	}

	[mutating]
	void appendOneChar(char_t c)
	{
		this[size()] = c;
		++_size;
	}

	[mutating]
	void appendChar(char_t c)
	{
		appendOneChar(c);
	}

	[mutating]
	void appendArray(const in IArray<char_t> array)
	{
		var writer = getWriter();
		const uint count = min(remainingCapacity(), array.getCount());
		for(uint i = 0; i < count; ++i)
		{
			write(writer, array[i]);
		}
		flushIFN(writer);
		_size += count;
	}

	[mutating]
	__generic <typename OtherStorage : IRWArray<Chunk>>
	void appendSameString(const in BasicStringBase<char_t, Chunk, OtherStorage> other)
	{
		const uint chunk_offset = size() % CharsPerChunk();
		if(chunk_offset == 0) // Aligned copy
		{
			const uint chunks_to_copy = other.activeChunks();
			const uint offset = size() / CharsPerChunk();
			const uint remaining_chunks = remainingCapacity() / CharsPerChunk();
			for(uint i = 0; i < min(remaining_chunks, chunks_to_copy); ++i)
			{
				_chunks[offset + i] = other._chunks[i];
			}
			_size += min(remainingCapacity(), other.size());
		}
		else
		{
			var reader = other.getReader();
			PendingChunk writer = getWriter();
			const uint chars_to_copy = min(remainingCapacity(), other.size());
			for(uint i = 0; i < chars_to_copy; ++i)
			{
				const char_t c = other.read(reader);
				write(writer, c);
			}
			flushIFN(writer);
			_size += chars_to_copy;
		}
	}

	[mutating]
	__generic <OtherString : IGenericString1>
	void appendString(const in OtherString str)
	{
		if(OtherString is BasicStringBase<char_t, Chunk, Storage>)
		{
			appendSameString((str as This).value);
		}
		else
		{
			// Can't do anything, show throw some kind of error
		}
	}

	[OverloadRank(15)]
	[mutating]
	__generic <typename OtherStorage : IRWArray<Chunk>>
	void appendString(const in BasicStringBase<char_t, Chunk, OtherStorage> other)
	{
		appendSameString(other);
	}

	static char_t GetASCII(uint digit, uint above_10_base = CHARCODE_hex_base)
	{
		if(digit < 10)	return char_t(CHARCODE_0 + digit);
		else			return char_t(digit - 10 + above_10_base);
	}

	[mutating]
	__generic <typename UInt : __BuiltinIntegerType>
	void appendUInt(inout ReversePendingChunk writer, UInt n, UInt basis, uint len)
	{
		for(uint i = 0; i < len; ++i)
		{
			const uint digit = reinterpret<uint>(n % basis);
			n = n / basis;
			const char_t ascii = GetASCII(digit);
			write(writer, ascii);
		}
	}

	// Can't express it, but requires UInt to be unsigned
	[mutating]
	__generic <typename UInt : __BuiltinIntegerType>
	void appendUInt(UInt n, uint basis, uint len)
	{
		_size += len;	
		var writer = getReverseWriter();
		appendUInt(writer, n, UInt(basis), len);
		flushIFN(writer);
	}

	[mutating]
	__generic <typename UInt : __BuiltinIntegerType>
	void appendUnsignedInteger(UInt n, const in IntegerFormat format)
	{	
		if(format.add_basis_prefix)
		{
			if(format.basis == 2 || format.basis == 8 || format.basis == 16)
			{
				var writer = getWriter();
				write(writer, char_t(CHARCODE_0));
				if(format.basis == 2 || format.basis == 16)
				{
					const char_t c = (format.basis == 2) ? char_t(CHARCODE_a + 1) : char_t(CHARCODE_x);
					write(writer, c);
				}
				flushIFN(writer);
			}
		}
		uint len = format.HowManyDigits(n);
		len = min(len, remainingCapacity());
		appendUInt(n, format.basis, len);
	}

	[mutating]
	__generic <typename Int : __BuiltinIntegerType>
	void appendInteger(Int n, const in IntegerFormat format)
	{
		if(IS_BUILTIN_UNSIGNED_INT(Int))
		{
			appendUnsignedInteger(n, format);
		}
		else
		{
			const bool neg = (n < Int(0));
			n = abs(n);
			if(neg)
			{
				appendOneChar(char_t(CHARCODE_minus));
			}
			else if(format.show_plus)
			{
				appendOneChar(char_t(CHARCODE_plus));
			}
			appendUnsignedInteger(n, format);
		}
	}

	[mutating]
	__generic <Float : __BuiltinFloatingPointType>
	void appendFloat(Float f, const in FloatFormat format)
	{
		const uint basis = 10;
		const bool neg = ExtractSignBit(f);
		if(isnan(f))
		{
			// TODO
			//append("NaN");
			appendOneChar(char_t(0x4e));
			appendOneChar(char_t(0x61));
			appendOneChar(char_t(0x4e));
		}
		if(neg)
		{
			appendOneChar(char_t(CHARCODE_minus));
		}
		else if(format.show_plus)
		{
			appendOneChar(char_t(CHARCODE_plus));
		}
		if(isinf(f))
		{
			// TODO
			//append("inf");
			appendOneChar(char_t(0x69));
			appendOneChar(char_t(0x6e));
			appendOneChar(char_t(0x66));
		}
		else
		{
			uint integral_part = uint(abs(f).toFloat()); //ConvertBuiltInTo<uint>(abs(f));
			const Float dec = abs(f - trunc(f));
			const float pow_mult = pow(basis, format.precision);
			const Float dec_f = dec * Float(pow_mult);
			uint dec_part = uint(dec_f.toFloat());
			IntegerFormat intf = {};
			intf.basis = basis;
			intf.digits = 0;
			intf.add_basis_prefix = false;
			intf.show_plus = false;
			appendUnsignedInteger(integral_part, intf);
			appendOneChar(char_t(CHARCODE_dot));
			appendUInt(dec_part, basis, format.precision);
		}
	}


	[mutating]
	void append(const in IPrintable printable)
	{
		printable.getPrintableImpl().appendToString(this);
	}

	uint estimateStringSize()
	{
		return size();
	}

	__generic <String : IGenericString1>
	void appendToString(inout String str)
	{
		str.appendString(this);
	}
};

__generic <typename char_t : IChar>
typealias DefaultChunk = UIntChunk<char_t, uint32_t>;

__generic <typename char_t : IChar, int Chunks, typename Chunk : IChunk<char_t> = DefaultChunk<char_t>>
struct LocalBasicString : BasicStringBase<char_t, Chunk, Array<Chunk, Chunks>>
{
	__init()
	{
		_size = 0;
	}
};


__generic <int Chunks = SHADER_STRING_PACKED_CAPACITY>
typealias LocalString = LocalBasicString<char, Chunks, DefaultChunk<char>>;




__generic <Int : __BuiltinIntegerType>
struct FormattedInteger : IPrintableImpl
{
	Int value;
	IntegerFormat format;

	__init(Int value)
	{
		this.value = value;
		format = {};
		format.basis = u8(10);
		format.digits = i8(0);
	}

	uint estimateStringSize()
	{
		uint smaller_basis_po2 = BitFloor(uint(format.basis));
		bool neg = IS_BUILTIN_SIGNED_INT(Int) && (value < Int(0));
		bool show_sign = neg || format.show_plus;
		Int a = Abs(value);
		uint d = format.HowManyDigitsFastConservative(value);
		uint res = d + (show_sign ? 1 : 0);
		if(format.add_basis_prefix)
		{
			if(format.basis == 2 || format.basis == 16)	res += 2;
			else if(format.basis == 8)					res += 1;
		}
		return res;
	}

	__generic <String : IGenericString1>
	void appendToString(inout String str)
	{
		str.appendInteger(value, format);
	}
};

__generic <Float : __BuiltinFloatingPointType>
struct FormattedFloat : IPrintableImpl
{
	Float value;
	FloatFormat format;

	__init(Float f)
	{
		value = f;
		format = {};
	}

	uint estimateStringSize()
	{
		const bool is_nan = isnan(value);
		if(is_nan)
		{
			return 3;
		}
		else
		{
			uint res = 0;
			const bool neg = ExtractSignBit(value);	
			if(format.show_plus || neg)	++res;
			const bool is_inf = isinf(value);
			if(is_inf)
			{
				res += 3;
			}
			else
			{
				res += format.precision;
				uint integral = uint(Abs(value).toFloat());
				res += FormattedInteger<uint>(integral).estimateStringSize();
				res += 1; // .
			}
			return res;
		}
	}

	__generic <String : IGenericString1>
	void appendToString(inout String str)
	{
		str.appendFloat(value, format);
	}
};

__generic <Int : __BuiltinIntegerType>
extension Int : IPrintable 
{
	typealias PrintableImpl = FormattedInteger<Int>;
	PrintableImpl getPrintableImpl()
	{ 
		return PrintableImpl(this);
	} 
};

__generic <Float : __BuiltinFloatingPointType>
extension Float : IPrintable 
{
	typealias PrintableImpl = FormattedFloat<Float>;
	PrintableImpl getPrintableImpl()
	{ 
		return PrintableImpl(this);
	} 
};


// TODO Add a common element format
__generic <typename P : IPrintable, typename A : IArray<P>>
struct FormattedArray : IPrintableImpl
{
	A value;
	Vector2<char> brackets;

	static Vector2<char> GetBracket(uint bracket_type)
	{
		Vector2<char> res;
		switch(bracket_type)
		{
			case 1:
				res = Vector2<char>(char(CHARCODE_lpar), char(CHARCODE_rpar));
			break;
			case 2:
				res = Vector2<char>(char(CHARCODE_lbracket), char(CHARCODE_rbracket));
			break;
			case 3:
				res = Vector2<char>(char(CHARCODE_lbrace), char(CHARCODE_rbrace));
			break;
			case 4:
				res = Vector2<char>(char(CHARCODE_lchevron), char(CHARCODE_rchevron));
			break;
			default:
				res = Vector2<char>(char(0), char(0)); // Means no brackets
			break;
		}
		return res;
	}

	__init(const in A value, uint bracket_type = 2)
	{
		this.value = value;
		brackets = GetBracket(bracket_type);
	}

	uint estimateStringSize()
	{
		uint dims = value.getCount();
		uint res = bit_cast<u16>(brackets) != u16(0) ? 2 : 0; // brackets
		if(dims > 0)
		{
			res += (dims - 1) * 2; // ", " between each element
		}
		
		for(uint i = 0; i < dims; ++i)
		{
			res += value[i].getPrintableImpl().estimateStringSize();
		}

		return res;
	}

	__generic <String : IGenericString1>
	void appendToString(inout String str)
	{
		if(brackets.x != char(0))
			str.appendChar(reinterpret<String::CharType>(brackets.x));
		const uint n = value.getCount();
		if(n != 0)
		{
			for(uint i = 0; i < n; ++i)
			{
				value[i].getPrintableImpl().appendToString(str);
				if(i != (n - 1))
				{
					Array<String::CharType, 2> sep = {String::CharType(CHARCODE_comma), String::CharType(CHARCODE_space)};
					str.appendArray(sep);
				}
			}
		}
		if(brackets.y != char(0))
			str.appendChar(reinterpret<String::CharType>(brackets.y));
	}

	typealias PrintableImpl = This;
	PrintableImpl getPrintableImpl()
	{
		return this;
	}
};



__generic<P : IPrintable, int N>
extension vector<P, N> : IPrintable
{
	typealias PrintableImpl = FormattedArray<P, vector<P, N>>;
	PrintableImpl getPrintableImpl()
	{
		return PrintableImpl(this);
	}
};




