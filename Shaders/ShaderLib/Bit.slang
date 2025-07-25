#pragma once

#include "common.slang"

// Don't know how to de better right now
#define __BuiltinUnsignedIntegerType __BuiltinIntegerType
#define __BuiltinSignedIntegerType __BuiltinIntegerType


template <typename Float : __BuiltinFloatingPointType, typename Int : __BuiltinIntegerType>
Float FloatOffset(Float f, Int o)
{
	if(Float is f16)
	{
		i16 i = bit_cast<i16>(f);
		i += ConvertBuiltInTo<i16>(o);
		f = bit_cast<Float>(i);
	}
	else if(Float is f32)
	{
		i32 i = bit_cast<i32>(f);
		i += ConvertBuiltInTo<i32>(o);
		f = bit_cast<Float>(i);
	}
	else if (Float is f64)
	{
		i64 i = bit_cast<i64>(f);
		i += ConvertBuiltInTo<i64>(o);
		f = bit_cast<Float>(i);
	}
	return f;
}

f16 f16Offset(f16 f, int16_t o)
{
	return FloatOffset(f, o);
}

f32 f32Offset(f32 f, int32_t o)
{
	return FloatOffset(f, o);
}

f64 f64Offset(f64 f, int64_t o)
{
	return FloatOffset(f, o);
}

__generic<int32 : __BuiltinInt32Type = uint32_t>
Vector2<int32> UnPack64To2x32(__BuiltinInt64Type u)
{
	Vector2<int32> v = Vector2<int32>(
		reinterpret<int32>(u),
		reinterpret<int32>(u >> 32),
	);
	return v;
}

__generic<int64 : __BuiltinInt64Type = u64, int32 : __BuiltinInt32Type = u32>
int64 Pack2x32To64(Vector2<int32> v)
{
	u64 res = u64(bit_cast<u32>(v.x)) | (u64(bit_cast<u32>(v.y)) << 32);
	return bit_cast<int64>(res);
}

__generic<typename UInt : __BuiltinUnsignedIntegerType>
constexpr uint CountBits(UInt value)
{
	if(IS_BUILTIN_SIZED_ANY_INTEGER(UInt, 64))
	{
		const uvec2 v = UnPack64To2x32<u32>(bit_cast<u64>(value));
		return countbits(v.x) + countbits(v.y);
	}
	else
	{
		return countbits(ConvertBuiltInTo<uint>(value));
	}
}

namespace impl
{
	__generic<typename Int : __BuiltinInt32Type>
	constexpr int FirstBitHigh_(Int i)
	{
		if(Int is i32)	return firstbithigh(bit_cast<i32>(i));
		else 			return firstbithigh(bit_cast<u32>(i));
	}

	__generic<typename Int : __BuiltinInt32Type>
	constexpr int FirstBitLow_(Int i)
	{
		if(Int is i32)	return firstbitlow(bit_cast<i32>(i));
		else 			return firstbitlow(bit_cast<u32>(i));
	}

	// Int and int32 should have the same signed-ness
	__generic<typename Int : __BuiltinIntegerType, int32 : __BuiltinInt32Type>
	constexpr int FirstBitHigh(Int value)
	{
		if(IS_BUILTIN_SIZED_ANY_INTEGER(Int, 64))
		{
			const Vector2<int32> v = UnPack64To2x32<int32>(bit_cast<u64>(value));
			int a = FirstBitHigh_(v.x);
			int res = a;
			if(a == -1)
			{
				res = FirstBitHigh_(v.y);
				if(res != -1)
				{
					res += 32;
				}
			}
			return res;
		}
		else
		{
			return FirstBitHigh_(ConvertBuiltInTo<int32>(value));
		}
	}

	// Int and int32 should have the same signed-ness
	__generic<typename Int : __BuiltinIntegerType, int32 : __BuiltinInt32Type>
	constexpr int FirstBitLow(Int value)
	{
		if(IS_BUILTIN_SIZED_ANY_INTEGER(Int, 64))
		{
			const Vector2<int32> v = UnPack64To2x32<int32>(bit_cast<u64>(value));
			int a = FirstBitLow_(v.x);
			int res = a;
			if(a == -1)
			{
				res = FirstBitLow_(v.y);
				if(res != -1)
				{
					res += 32;
				}
			}
			return res;
		}
		else
		{
			return FirstBitLow_(ConvertBuiltInTo<int32>(value));
		}
	}
}

// Return the bit index of the most significant high bit
// If no high bit is found, return `-1`
// For positive values, a high bit is a `1`
// For negative values, a high bit is a `0`
// `0b1010` returns `3`
__generic<typename Int : __BuiltinIntegerType>
constexpr int FirstBitHigh(Int value)
{
	if(IS_BUILTIN_SIGNED_INT(Int))
	{
		return impl::FirstBitHigh<Int, i32>(value);
	}
	else
	{
		return impl::FirstBitHigh<Int, u32>(value);
	}
}

__generic<typename Int : __BuiltinIntegerType>
constexpr int FirstBitLow(Int value)
{
	if(IS_BUILTIN_SIGNED_INT(Int))
	{
		return impl::FirstBitLow<Int, i32>(value);
	}
	else
	{
		return impl::FirstBitLow<Int, u32>(value);
	}
}

__generic<typename UInt : __BuiltinUnsignedIntegerType>
constexpr UInt ReverseBits(UInt value)
{
	UInt res;
	if(IS_BUILTIN_SIZED_ANY_INTEGER(UInt, 64))
	{
		uvec2 v = UnPack64To2x32<u32>(bit_cast<u64>(value));
		v = reversebits(v).yx;
		res = bit_cast<UInt>(Pack2x32To64<u64>(v));
	}
	else
	{
		uint v = ConvertBuiltInTo<uint>(value);
		v = reversebits(v);
		v = (v >> (8 * (sizeof(uint) - sizeof(UInt))));
		res = ConvertBuiltInTo<UInt>(v);
	}
	return res;
}





__generic<typename UInt : __BuiltinIntegerType>
constexpr bool IsPowerOf2(UInt i)
{
	return ((i - UInt(1)) & i) == UInt(0);
}

__generic<typename UInt : __BuiltinIntegerType>
constexpr bool IsPo2(UInt i)
{
	return IsPowerOf2(i);
}

__generic<typename UInt : __BuiltinIntegerType>
constexpr UInt AlignUp(UInt n, UInt a)
{
	const UInt r = n % a;
	return (r != UInt(0)) ? (n + a - r) : n;
}

__generic<typename UInt : __BuiltinIntegerType>
constexpr UInt AlignUpAssumePo2(UInt n, UInt a)
{
	const UInt a_mask = a - UInt(1);
	return (n + a_mask) & ~a_mask;
}

__generic<typename UInt : __BuiltinIntegerType>
constexpr UInt AlignDown(UInt n, UInt a)
{
	return n - (n % a);
}

__generic<typename UInt : __BuiltinIntegerType>
constexpr UInt AlignDownAssumePo2(UInt n, UInt a)
{
	const UInt a_mask = a - UInt(1);
	return n & ~a_mask;
}

namespace impl
{
	
}

// Returns the smallest number of bits needed to represent `n`
// Only works with unsigned integers
__generic <typename UInt : __BuiltinUnsignedIntegerType>
constexpr uint BitWidth(UInt n)
{
	uint res = 0;
	if(n != UInt(0))
	{
		res = FirstBitHigh(n) + 1;
	}
	return res;
}

// Returns the largest `UInt` that is a power of two, and `<= n`
// `BitFloor(0) = 0`
// `BitFloor(0b0110) = 0b0100` 
__generic <typename UInt : __BuiltinIntegerType>
constexpr UInt BitFloor(UInt n)
{
	if(n == UInt(0))	return UInt(0);
	else				return UInt(1) << (FirstBitHigh(n));
}

// Returns the smallest `UInt` that is a power of two, and `>= n`, if representable
// `BitCeil(0) = 1`
// `BitCeil(0b0110) = 0b1000` 
__generic <typename UInt : __BuiltinIntegerType>
constexpr UInt BitCeil(UInt n)
{
	uint s = BitWidth(n);
	if(!IsPo2(n))	++s;
	return UInt(1) << s;
}

__generic<typename UInt : IInteger>
constexpr UInt RotL(UInt v, uint n)
{
	const uint N = sizeof(UInt) * 8;
	return (v.shl(n)).bitOr(v.shr(N - n));
}

__generic<typename UInt : IInteger>
constexpr UInt RotR(UInt v, uint n)
{
	const uint N = sizeof(UInt) * 8;
	return (v.shr(n)).bitOr(v.shl(N - n));
}

// Returns the smallest `m >= n` such that `m` is a power of two
// If `n` is already a power of two, returns `n`
__generic<typename UInt : __BuiltinIntegerType>
constexpr UInt RoundToUpperPo2(UInt n)
{
	UInt res = BitCeil(n);
	return res;
}

__generic<typename UInt : IInteger>
constexpr UInt DivDown(UInt num, UInt div)
{
	return num.div(div);
}

__generic<typename UInt : __BuiltinIntegerType>
constexpr UInt DivDownAssumePo2(UInt num, UInt div_po2)
{
	uint s = FirstBitHigh(div_po2);
	return num.shr(s);
}

__generic<typename UInt : IInteger>
constexpr UInt DivUpAssumeNoOverflow(UInt num, UInt div)
{
	return (num + (div - UInt(1))).div(div);
}

__generic<typename UInt : IInteger>
constexpr UInt DivUpSafe(UInt num, UInt div)
{
	UInt res = DivDown(num, div);
	UInt remainder = num.mod(div);
	if(remainder != UInt(0))	res = res + UInt(1);
	return res;
}

__generic<typename UInt : __BuiltinIntegerType>
constexpr UInt DivUpAssumePo2(UInt num, UInt div_po2)
{
	UInt mask = div_po2 - UInt(1);
	UInt res = DivDownAssumePo2(num + mask, div_po2);
	return res;
}


// Positive returns `false` 
// Negarive returns `true`
__generic <typename Float : __BuiltinFloatingPointType>
constexpr bool ExtractSignBit(Float f)
{
	if(f is f16)
	{
		return bool(bitfieldExtract(bit_cast<u16>(f), 8 * sizeof(f) - 1, 1));
	}
	else if(f is f32)
	{
		return bool(bitfieldExtract(bit_cast<u32>(f), 8 * sizeof(f) - 1, 1));
	}
	else if(f is f64)
	{
		return bool(bitfieldExtract(bit_cast<u64>(f), 8 * sizeof(f) - 1, 1));
	}
	return false;
}

__generic <typename Int : __BuiltinIntegerType>
constexpr bool ExtractSignBit(Int n)
{
	if(IS_BUILTIN_UNSIGNED_INT(Int))
	{
		return false;
	}
	else
	{
		return (n & Int(1 << (8 * sizeof(Int) - 1))) != Int(0);
	}
}

// Find the bit index (from LSB to MSB) of the `n`-th high bit
// TODO implement it with a dichotomic search
__generic<UInt : __BuiltinIntegerType>
uint FindHighBitIndex(UInt bits, uint n, uint max_bit)
{
	for(uint i = 0; i < max_bit; ++i)
	{
		if((bits & (UInt(1) << UInt(i))) != UInt(0))
		{
			if(n == 0)
			{
				return i;
			}
			--n;
		}
	}
	return uint(-1);
}

// Find the bit index (from LSB to MSB) of the `n`-th high bit
__generic<UInt : __BuiltinIntegerType>
uint FindHighBitIndex(UInt bits, uint n)
{
	return FindHighBitIndex(bits, n, 8 * sizeof(UInt));
}

// Returns n ^ p
// https://stackoverflow.com/a/101613
__generic<UInt : __BuiltinIntegerType>
UInt powu(UInt n, UInt p)
{
	UInt res = UInt(1);
	do
	{
		if((p & UInt(1)) != UInt(0))
			res *= n;
		p >>= UInt(1);
		n *= n;
	} while(p != UInt(0));
	return res;
}

