#pragma once

#include <ShaderLib/common.slang>
#include "TemplateImpl/default_hash.inl"


typealias fast_hash_t = uint32_t;
typealias slow_hash_t = uint64_t;

#if DEFAULT_HASH_POLICY == HASH_POLICY_FAST
typealias hash_t = fast_hash_t;
#else
typealias hash_t = slow_hash_t;
#endif

#define IHashResult __BuiltinIntegerType

enum HashPolicy
{
	Fast = 0,
	Slow = 1,
#if DEFAULT_HASH_POLICY == HASH_POLICY_FAST
	Default = HashPolicy::Fast,
#else
	Default = HashPolicy::Slow,
#endif
};


fast_hash_t FastHash32(uint32_t x)
{
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = (x >> 16) ^ x;
	return x;
}

fast_hash_t FastHash64(uint64_t x)
{
	return FastHash32(uint32_t(x)) ^ FastHash32(uint32_t(x >> 32));
}

template <typename UInt : __BuiltinIntegerType>
fast_hash_t FastHashIntegral(UInt x)
{
	fast_hash_t res;
	if (UInt is __BuiltinInt64Type)
	{
		res = FastHash64(reinterpret<uint64_t>(x));
	}
	else
	{
		uint32_t y = ConvertBuiltInTo<uint32_t>(x);
		res = FastHash32(y);
	}
	return res;
}

slow_hash_t SlowHash64(uint64_t x)
{
	x = ((x >> 33) ^ x) * 0xff51afd7ed558ccd;
	x = ((x >> 33) ^ x) * 0xc4ceb9fe1a85ec53;
	x = (x >> 33) ^ x;
	return x;
}

template <typename UInt : __BuiltinIntegerType>
slow_hash_t SlowHashIntegral(UInt x)
{
	return SlowHash64(ConvertBuiltInTo<uint64_t>(x));
}

template <typename T_Hash_t : IHashResult = hash_t, typename UInt : __BuiltinIntegerType>
T_Hash_t HashIntegral(UInt x)
{
	T_Hash_t res;
	if(T_Hash_t is fast_hash_t)
	{
		res = T_Hash_t(FastHashIntegral(x));
	}
	else if(T_Hash_t is slow_hash_t)
	{
		res = T_Hash_t(SlowHashIntegral(x));
	}
	return res;
}

template <typename T_Hash_t : IHashResult = hash_t, typename Float : __BuiltinFloatingPointType>
T_Hash_t HashFloat(Float x)
{
	T_Hash_t res;
	if(Float is f32)
	{
		res = HashIntegral<T_Hash_t>(reinterpret<u32>(x));
	}
	else if(Float is f64)
	{
		res = HashIntegral<T_Hash_t>(reinterpret<u64>(x));
	}
	else if(Float is f16)
	{
		res = HashIntegral<T_Hash_t>(reinterpret<u16>(x));
	}
	return res;
}

template <typename T_Hash_t : IHashResult = hash_t, typename T : __BuiltinArithmeticType>
T_Hash_t Hash_(T t)
{
	T_Hash_t res;
	if(T is __BuiltinIntegerType)
	{
		res = HashIntegral<T_Hash_t>((t as __BuiltinIntegerType).value);
	}
	else if(T is __BuiltinFloatingPointType)
	{
		res = HashFloat<T_Hash_t>((t as __BuiltinFloatingPointType).value);
	}
	return res;
}

template <typename T_Hash_t : IHashResult = hash_t, typename T : __BuiltinArithmeticType, int N>
T_Hash_t Hash_(const in Vector<T, N> vec)
{
	T_Hash_t res = T_Hash_t(0);
	for(uint i = 0; i < N; ++i)
	{
		res = res ^ Hash_<T_Hash_t>(vec[i]);
	}
	return res;
}

template <typename T_Hash_t : IHashResult = hash_t, typename T : __BuiltinArithmeticType, int R, int C>
T_Hash_t Hash_(const in Matrix<T, R, C> mat)
{
	T_Hash_t res = T_Hash_t(0);
	for(uint i = 0; i < R; ++i)
	{
		res = res ^ Hash_<T_Hash_t>(mat[i]);
	}
	return res;
}

// TODO better
// Right now, it appears to be the best option for specialization since the generics at too restricitve right now
#define Hash Hash_<hash_t>
#define FastHash Hash_<fast_hash_t>
#define SlowHash Hash_<slow_hash_t>
