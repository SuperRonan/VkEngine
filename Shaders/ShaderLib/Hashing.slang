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
	if (sizeof(UInt) == 8)
	{
		res = FastHash64(bit_cast<uint64_t>(x));
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
	x = ((x >> 33) ^ x) * u64(0xff51afd7ed558ccd);
	x = ((x >> 33) ^ x) * u64(0xc4ceb9fe1a85ec53);
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
		res = bit_cast<T_Hash_t>(FastHashIntegral(x));
	}
	else if(T_Hash_t is slow_hash_t)
	{
		res = bit_cast<T_Hash_t>(SlowHashIntegral(x));
	}
	return res;
}

template <typename T_Hash_t : IHashResult = hash_t, typename Float : __BuiltinFloatingPointType>
T_Hash_t HashFloat(Float x)
{
	T_Hash_t res;
	if(Float is f32)
	{
		res = HashIntegral<T_Hash_t>(bit_cast<u32>(x));
	}
	else if(Float is f64)
	{
		res = HashIntegral<T_Hash_t>(bit_cast<u64>(x));
	}
	else if(Float is f16)
	{
		res = HashIntegral<T_Hash_t>(bit_cast<u16>(x));
	}
	return res;
}

template <typename T_Hash_t : IHashResult = hash_t, typename T : __BuiltinArithmeticType>
T_Hash_t Hash_(T t)
{
	T_Hash_t res;
	if(IS_BUILTIN_INTEGER(T))
	{
		if(sizeof(T_Hash_t) >= sizeof(T))
		{
			res = HashIntegral<T_Hash_t>(ConvertBuiltInTo<T_Hash_t>(t));
		}
		else
		{
			res = HashIntegral<T_Hash_t>(bit_cast<u64>(t));
		}
	}
	else if(IS_BUILTIN_FLOAT(T))
	{
		if(T is f32)
		{
			res = HashIntegral<T_Hash_t>(bit_cast<u32>(t));
		}
		else if(T is f64)
		{
			res = HashIntegral<T_Hash_t>(bit_cast<u64>(t));
		}
		else if(T is f16)
		{
			res = HashIntegral<T_Hash_t>(bit_cast<u16>(t));
		}
	}
	return res;
}

// There are two ways to Hash a Scalar sequence:
// Sequencial: res[i] = H(res[i-1] ^ H(vec[i]))
// Recursive: H(vec4 v) = H(V.xy) ^ H(V.zw)

template <typename T_Hash_t : IHashResult = hash_t, typename T : __BuiltinArithmeticType, typename Array : IArray<T>>
T_Hash_t HashSequencial(const in Array array)
{
	T_Hash_t res = T_Hash_t(0);
	const uint N = array.getCount();
	for(uint i = 0; i < N; ++i)
	{
		res = Hash_<T_Hash_t>(res ^ Hash_<T_Hash_t>(array[i]));
	}
	return res;
}

template <typename T_Hash_t : IHashResult = hash_t, typename T : __BuiltinArithmeticType, typename Array : IArray<T>>
T_Hash_t Hash_(const in Array array)
{
	return HashSequencial<T_Hash_t>(array);
}

// TODO better
// Right now, it appears to be the best option for specialization since the generics at too restricitve right now
#define Hash Hash_<hash_t>
#define FastHash Hash_<fast_hash_t>
#define SlowHash Hash_<slow_hash_t>
