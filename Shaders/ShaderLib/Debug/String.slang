#pragma once

#include <ShaderLib/common.slang>

#include "StringDefinitions.h"

#include <ShaderLib/Bit.slang>

#ifndef GLOBAL_ENABLE_SLANG_DEBUG
#define GLOBAL_ENABLE_SLANG_DEBUG GLOBAL_ENABLE_SHADER_DEBUG
#endif

typealias IChar = __BuiltinIntegerType;
typealias char = uint8_t;

char GetASCII(uint digit, uint above_10_base = CHARCODE_hex_base)
{
	if(digit < 10)	return char(CHARCODE_0 + digit);
	else			return char(digit - 10 + above_10_base);
}

__generic<typename UInt : __BuiltinIntegerType>
struct DigitCount
{
	uint digits;
	UInt max_power_basis;
}

__generic <typename UInt : __BuiltinIntegerType>
DigitCount<UInt> HowManyDigitsAnyBasis(UInt n, UInt base)
{
	DigitCount<UInt> res = {1, UInt(1)};
	n /= base;
	while(n != UInt(0))
	{
		n /= base;
		++res.digits;
		res.max_power_basis *= base;
	}
	return res;
}

__generic<typename UInt : __BuiltinIntegerType>
uint HowManyDigitsAssumePo2Basis(UInt n, uint basis)
{
	uint w = BitWidth(n);
	uint d = (FirstBitHigh(basis));
	return max(DivUpSafe(w, d), 1);
}

__generic<typename UInt : __BuiltinIntegerType>
UInt GetMaxDigitPowerPo2Basis(UInt digits, UInt basis)
{
	return (UInt(1) << ((digits - UInt(1)) * UInt(FirstBitHigh(basis)))); // Not 100% sure of it, TODO check
}



__generic <typename UInt : __BuiltinIntegerType>
DigitCount<UInt> HowManyDigits(UInt n, uint basis)
{
	DigitCount<UInt> res;
	if(IsPo2(basis))
	{
		res.digits = HowManyDigitsAssumePo2Basis(n, basis);
		res.max_power_basis = GetMaxDigitPowerPo2Basis(UInt(res.digits), UInt(basis));
	}
	else
		res = HowManyDigitsAnyBasis(n, UInt(basis));
	return res;
}


__generic <typename char_t : IChar>
extension char_t
{
	__init(uint c)
	{
		this = char_t(int(c));
	}
}

__generic <char_t : IChar>
interface IChunk
{
	// in chars
	static constexpr uint Capacity();

	__init();

	//static_assert(Capacity() >= sizeof(char_t), "A chunk must be able to hold at least one char.");

	[mutating]
	void clear();

	__subscript(uint index) -> char_t
	{
		get;
		set;
	}

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

	[mutating]
	void setAssumeClear(uint index, char_t char)
	{
		UInt v = reinterpret<UInt>(char) << (sizeof(char_t) * 8 * index);
		u = u | v;
	}

	__subscript(uint index) -> char_t
	{
		get {
			const UInt masked = (u >> (sizeof(char_t) * 8 * index)) & CharMask();
			return reinterpret<char_t>(masked);
		}
		set {
			u = u & ~CharMask(index);
			setAssumeClear(index, newValue);
		}
	}
}

__generic<typename char_t : IChar>
interface IBasicStringBase : IRWArray<char_t>
{
	property uint length
	{
		get;
		set;
	}

	associatedtype Caret;

	Caret getReader(uint base = 0);

	Caret getWriter(uint base);

	Caret getWriter();

	char_t read(inout Caret caret);

	[mutating]
	void write(inout Caret caret, char_t value);

	[mutating]
	__generic<int N>
	void write(inout Caret caret, const in ShortString<N> str);

	[mutating]
	void flushIFN(inout Caret caret);
};

__generic <
	typename char_t : IChar,
	typename Chunk : IChunk<char_t>,
	typename Storage : IRWArray<Chunk>
>
struct BasicStringBase : IBasicStringBase<char_t>
{
	typealias CharType = char_t;

	// chars are manually packed as uint because the optimizer tends to store each individual char in a uint
	Storage _chunks;
	uint _len = 0;

	property uint length
	{
		get { return _len; }
		set { _len = newValue; }
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
		return _len;
	}

	constexpr uint remainingCapacity()
	{
		return capacity() - _len;
	}

	constexpr uint activeChunks()
	{
		return (_len + CharsPerChunk() - 1) / CharsPerChunk();
	}

	constexpr __subscript(uint i) -> char_t
	{
		get {
			const uint chunk_index = i / CharsPerChunk(); 
			const uint index_in_chunk = i % CharsPerChunk(); 
			return _chunks[chunk_index][index_in_chunk];
		}
		set {
			const uint chunk_index = i / CharsPerChunk(); 
			const uint index_in_chunk = i % CharsPerChunk(); 
			_chunks[chunk_index][index_in_chunk] = newValue;
		}
	}

	struct PendingChunkImpl
	{
		uint index = 0;
		Chunk chunk = {};

		uint getIndex()
		{
			return index;
		}

		__subscript(uint index_in_chunk) -> char_t
		{
			get {
				return chunk[index_in_chunk];
			}
			set {
				chunk[index_in_chunk] = newValue;
			}
		}
	};

	typealias PendingChunk = PendingChunkImpl;
	typealias Caret = PendingChunk;

	Caret getReader(uint base = 0)
	{
		PendingChunk res = {};
		res.index = base;
		if(base % CharsPerChunk() != 0)
		{
			res.chunk = _chunks[base / CharsPerChunk()];
		}
		return res;
	}

	Caret getWriter(uint base)
	{
		PendingChunk res;
		res.index = base;
		if(base > 0)
		{
			res.chunk = _chunks[res.index / CharsPerChunk()];
		}
		return res;
	}

	Caret getWriter()
	{
		return getWriter(_len);
	}

	char_t read(inout PendingChunk reader)
	{
		const uint id_in_chunk = reader.index % CharsPerChunk();
		if(id_in_chunk == 0)
		{
			const uint chunk_id = reader.index / CharsPerChunk();
			reader.chunk = _chunks[chunk_id];
		}
		const char_t res = reader.chunk[id_in_chunk];
		++reader.index;
		return res;
	}

	[mutating]
	void write(inout PendingChunk writer, char_t c)
	{
		const uint id_in_chunk = writer.index % CharsPerChunk();
		const uint chunk_id = writer.index / CharsPerChunk();
		writer.chunk[id_in_chunk] = c;
		if(id_in_chunk == (CharsPerChunk() - 1))
		{
			_chunks[chunk_id] = writer.chunk;
			writer.chunk.clear();
		}
		++writer.index;
	}

	[mutating]
	__generic<int N>
	void write(inout PendingChunk writer, const in ShortString<N> str)
	{
		for(uint i = 0; i < N; ++i)
		{
			write(writer, char_t(str[i]));
		}
	}

	[mutating]
	void flushIFN(inout PendingChunk writer)
	{
		const uint id_in_chunk = writer.index % CharsPerChunk();
		const uint chunk_id = writer.index / CharsPerChunk();
		if(id_in_chunk != 0)
		{
			_chunks[chunk_id] = writer.chunk;
		}
		_len = writer.index;
	}
};
interface IPrintableImpl
{
	uint estimateStringSize();

	__generic <typename char_t : IChar, String : IBasicStringBase<char_t>>
	void appendToString(inout String str, inout String::Caret writer);
};

__generic <PrintableImpl : IPrintableImpl>
extension PrintableImpl
{
	__generic <typename char_t : IChar, String : IBasicStringBase<char_t>>
	void appendToString(inout String str)
	{
		var writer = str.getWriter();
		appendToString(str, writer);
		str.flushIFN(writer);
	}
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

__generic <
	typename char_t : IChar,
	typename Chunk : IChunk<char_t>,
	typename Storage : IRWArray<Chunk>
>
extension BasicStringBase<char_t, Chunk, Storage>
{
	[mutating]
	__generic<P : IPrintable>
	void append(inout Caret writer, const in P p)
	{
		p.getPrintableImpl().appendToString(this, writer);
	}
}

__generic <typename char_t : IChar>
typealias DefaultChunk = UIntChunk<char_t, uint32_t>;


__generic<int N>
extension ShortString<N> : IPrintableImpl
{
	uint estimateStringSize()
	{
		return N;
	}

	__generic <typename char_t : IChar, String : IBasicStringBase<char_t>>
	void appendToString(inout String str, inout String::Caret writer)
	{
		str.write(writer, this);
	}
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

	// Returns `(n, pow(basis, n-1))`
	__generic<UInt : __BuiltinIntegerType>
	DigitCount<UInt> HowManyDigits(UInt value)
	{
		DigitCount<UInt> res;
		if(digits == 0)
		{
			res = ::HowManyDigits(value, uint(basis));
		}
		else
		{
			if(digits < 0)
			{
				res.digits = MaxDigitsForBasis<UInt>();
			}
			else
			{
				res.digits = digits;
			}
			if(IsPo2(uint(basis)))
			{
				res.max_power_basis = GetMaxDigitPowerPo2Basis(UInt(res.digits), UInt(basis));
			}
			else
			{
				res.max_power_basis = powu(UInt(basis), UInt(res.digits - 1));
			}
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

	__init(Int value, IntegerFormat format)
	{
		this.value = value;
		this.format = format;
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

	__generic <typename char_t : IChar, String : IBasicStringBase<char_t>>
	void appendUnsignedToString(Int n, inout String str, inout String::Caret writer)
	{
		if(format.add_basis_prefix)
		{
			if(format.basis == 2 || format.basis == 8 || format.basis == 16)
			{
				str.write(writer, char_t('0'));
				char_t p;
				switch(format.basis)
				{
					case 0x02: p = char_t('b'); break;
					case 0x08: p = char_t('o'); break;
					case 0x10: p = char_t('x'); break;
				}
				str.write(writer, p);
			}
		}
		DigitCount<Int> dc = format.HowManyDigits(n);
		// Assume the remaining capacity is enough
		for(uint i = 0; i < dc.digits; ++i)
		{
			const uint digit = (n / dc.max_power_basis).toUInt();
			n = n % dc.max_power_basis;
			dc.max_power_basis /= Int(format.basis);

			const char_t ascii = char_t(GetASCII(digit));
			str.write(writer, ascii);
		}
	}

	__generic <typename char_t : IChar, String : IBasicStringBase<char_t>>
	void appendToString(inout String str, inout String::Caret writer)
	{
		Int n = value;
		if(IS_BUILTIN_UNSIGNED_INT(Int))
		{
			appendUnsignedToString(n, str, writer);
		}
		else
		{
			const bool neg = n < Int(0);
			if(neg)
			{
				str.write(writer, char_t('-'));
			}
			else if(format.show_plus)
			{
				str.write(writer, char_t('+'));
			}
			appendUnsignedToString(abs(n), str, writer);
		}
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

	__generic <typename char_t : IChar, String : IBasicStringBase<char_t>>
	void appendToString(inout String str, inout String::Caret writer)
	{
		const uint basis = 10;
		const bool neg = ExtractSignBit(value);
		if(isnan(value))
		{
			str.write(writer, "NaN");
		}
		else
		{
			if(neg)
			{
				str.write(writer, char_t('-'));
			}
			else if(format.show_plus)
			{
				str.write(writer, char_t('+'));
			}
			if(isinf(value))
			{
				str.write(writer, "inf");
			}
			else
			{
				uint integral_part = uint(abs(value).toFloat()); //ConvertBuiltInTo<uint>(abs(f));
				const Float dec = abs(trunc(value) - value);
				const float pow_mult = floor(pow(basis, format.precision) + 0.5);
				const Float dec_f = dec * Float(pow_mult);
				uint dec_part = uint(dec_f.toFloat());
				IntegerFormat intf = {};
				intf.basis = basis;
				intf.digits = 0;
				intf.add_basis_prefix = false;
				intf.show_plus = false;
				FormattedInteger<uint>(integral_part, intf).appendUnsignedToString(integral_part, str, writer);
				str.write(writer, char_t('.'));
				intf.digits = i8(format.precision);
				FormattedInteger<uint>(dec_part, intf).appendUnsignedToString(dec_part, str, writer);
			}
		}
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

Vector2<char> GetBracketPair(uint bracket_type)
{
	Vector2<char> res;
	switch(bracket_type)
	{
		case 1:
			res = Vector2<char>(char('('), char(')'));
		break;
		case 2:
			res = Vector2<char>(char('['), char(']'));
		break;
		case 3:
			res = Vector2<char>(char('{'), char('}'));
		break;
		case 4:
			res = Vector2<char>(char('<'), char('>'));
		break;
		default:
			res = Vector2<char>(char(0), char(0)); // Means no brackets
		break;
	}
	return res;
}

// TODO Add a common element format
__generic <typename P : IPrintable, typename A : IArray<P>>
struct FormattedArray : IPrintableImpl
{
	A value;
	Vector2<char> brackets;

	__init(const in A value, uint bracket_type = 2)
	{
		this.value = value;
		brackets = GetBracketPair(bracket_type);
	}

	__init(const in A value, Vector2<char> brackets)
	{
		this.value = value;
		this.brackets = brackets;
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

	__generic <typename char_t : IChar, String : IBasicStringBase<char_t>>
	void appendToString(inout String str, inout String::Caret writer)
	{
		if(brackets.x != char(0))
			str.write(writer, char_t(brackets.x));
		const uint n = value.getCount();
		if(n != 0)
		{
			for(uint i = 0; i < n; ++i)
			{
				value[i].getPrintableImpl().appendToString(str, writer);
				if(i != (n - 1))
				{
					str.write(writer, ", ");
				}
			}
		}
		if(brackets.y != char(0))
			str.write(writer, char_t(brackets.y));
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


namespace fmt
{
	IntegerFormat ExplicitIntFormat(uint basis, int digits = -1)
	{
		IntegerFormat res;
		res.basis = u8(basis);
		res.digits = i8(digits);
		res.show_plus = false;
		res.add_basis_prefix = true;
		return res;
	}

	IntegerFormat BinFormat(int digits = -1)
	{
		return ExplicitIntFormat(0x02, digits);
	}

	IntegerFormat OctFormat(int digits = -1)
	{
		return ExplicitIntFormat(0x08, digits);
	}

	IntegerFormat HexFormat(int digits = -1)
	{
		return ExplicitIntFormat(0x10, digits);
	}

	__generic<Int : __BuiltinIntegerType>
	FormattedInteger<Int> Bin(Int value, int digits = -1)
	{
		FormattedInteger<Int> res;
		res.value = value;
		res.format = BinFormat(digits);
		return res;
	}

	__generic<Int : __BuiltinIntegerType>
	FormattedInteger<Int> Oct(Int value, int digits = -1)
	{
		FormattedInteger<Int> res;
		res.value = value;
		res.format = OctFormat(digits);
		return res;
	}

	__generic<Int : __BuiltinIntegerType>
	FormattedInteger<Int> Hex(Int value, int digits = -1)
	{
		FormattedInteger<Int> res;
		res.value = value;
		res.format = HexFormat(digits);
		return res;
	}
}