#pragma once

#include "Hashing.slang"

#ifndef DEFAULT_RNG_POLICY
#define DEFAULT_RNG_POLICY DEFAULT_HASH_POLICY
#endif

interface IRandomNumberGenerator
{
	[mutating]
	__generic<typename Scalar : __BuiltinArithmeticType>
	Scalar generate();

	[mutating]
	__generic<typename Scalar : __BuiltinArithmeticType>
	Scalar generate(Scalar min, Scalar length);

	[mutating]
	__generic <typename Scalar : __BuiltinArithmeticType, typename Target : IRWArray<Scalar>, typename Converter : IFunc<Scalar, Scalar>>
	void generate(inout Target target, uint N, const in Converter convert);

	[mutating]
	__generic <typename Scalar : __BuiltinArithmeticType, typename Target : IRWArray<Scalar>>
	void generate(inout Target target, uint N);

	[mutating]
	__generic <typename Scalar : __BuiltinArithmeticType, typename Target : IRWArray<Scalar>>
	void generate(inout Target target, uint N, Scalar min, Scalar length);

	[mutating]
	__generic <typename Scalar : __BuiltinArithmeticType, int N>
	Vector<Scalar, N> generate();

	[mutating]
	__generic <typename Scalar : __BuiltinArithmeticType, int N, typename Converter : IFunc<Scalar, Scalar>>
	Vector<Scalar, N> generate(const in Converter converter);

	[mutating]
	__generic <typename Scalar : __BuiltinArithmeticType, int N>
	Vector<Scalar, N> generate(Scalar min, Scalar length);
};

typealias IRNG = IRandomNumberGenerator;

namespace impl
{
	__generic <typename Scalar : __BuiltinArithmeticType>
	struct RNGConverter : IFunc<Scalar, Scalar>
	{
		Scalar min;
		Scalar length;

		__init()
		{
			min = Scalar(0);
			if(IS_BUILTIN_INTEGER(Scalar))
			{
				length = Scalar(0) - Scalar(1);
			}
			else if(IS_BUILTIN_FLOAT(Scalar))
			{
				length = Scalar(1);
			}
		}

		Scalar operator()(const Scalar s)
		{
			Scalar res;
			if(IS_BUILTIN_INTEGER(Scalar))
			{
				res = (s % length) + min;
			}
			else if(IS_BUILTIN_FLOAT(Scalar))
			{
				// The scalar is actually not a Float, but bit packed uint
				// So we need to convert it a Float representation
				Scalar maks;
				if(Scalar is f16)
				{
					const u16 mask = BitMask<u16>(12);
					maks = ConvertBuiltInTo<Scalar>(mask + 1);
					u16 u = bit_cast<u16>(s) & mask;
					res = ConvertBuiltInTo<Scalar>(u);
				}
				else if(Scalar is f32)
				{
					const u32 mask = BitMask<u32>(24);
					maks = ConvertBuiltInTo<Scalar>(mask + 1);
					u32 u = bit_cast<u32>(s) & mask;
					res = ConvertBuiltInTo<Scalar>(u);
				}
				else if(Scalar is f64)
				{
					const u64 mask = BitMask<u64>(48);
					maks = ConvertBuiltInTo<Scalar>(mask + 1);
					u64 u = bit_cast<u64>(s) & mask;
					res = ConvertBuiltInTo<Scalar>(u);
				}
				res = res * length / maks + min;

				// TODO explore this alternative of uniform float generation
				// if(Scalar is float)
				// {
				// 	uint u = bit_cast<uint>(s);
				// 	u = u & BitMask<uint>(23);
				// 	uint n = 0;
				// 	u = (u & ~BitMask<uint>(n));
				// 	float uf = bit_cast<float>(bit_cast<uint>(float(0.5f)) | u) - 0.5f;
				// 	res = bit_cast<Scalar>(uf * 2.0f);
				// 	res = res * length + min;
				// }
			}
			return res;
		}
	}
}

__generic <typename Hash_t : IHashResult>
struct RandomNumberGenerator : IRandomNumberGenerator
{
	Hash_t _seed;

	__generic <typename UInt :__BuiltinIntegerType>
	__init(UInt seed)
	{
		_seed = ConvertBuiltInTo<Hash_t>(seed);
	}

	[mutating]
	Hash_t roll()
	{
		Hash_t res = Hash_<Hash_t>(_seed);
		++this._seed;
		return res;
	}

	[mutating]
	u64 randomU64()
	{
		u64 res;
		if(Hash_t is fast_hash_t)
		{
			u32 a = bit_cast<u32>(roll());
			u32 b = bit_cast<u32>(roll());
			res = u64(a) << 32 | u64(b);
		}
		else if (Hash_t is slow_hash_t)
		{
			res = bit_cast<u64>(roll());
		}
		return res;
	}

	[mutating]
	__generic <typename Scalar : __BuiltinArithmeticType, typename Converter : IFunc<Scalar, Scalar>>
	Scalar generateImpl(const in Converter converter)
	{
		Scalar res;
		Scalar s;
		if(sizeof(Scalar) <= sizeof(Hash_t))
		{
			Hash_t h = roll();
			s = reinterpret<Scalar>(h);
		}
		else if(sizeof(Scalar) == 8)
		{
			const u64 u = randomU64();
			s = reinterpret<Scalar>(u);	
		}
		res = converter(s);
		return res;
	}

	[mutating]
	__generic<typename Scalar : __BuiltinArithmeticType>
	Scalar generate()
	{
		impl::RNGConverter<Scalar> converter = impl::RNGConverter<Scalar>();
		Scalar res;
		res = this.generateImpl<Scalar>(converter);
		return res;
	}

	[mutating]
	__generic<typename Scalar : __BuiltinArithmeticType>
	Scalar generate(Scalar min, Scalar length)
	{
		impl::RNGConverter<Scalar> converter;
		converter.min = min;
		converter.length = length;
		return this.generateImpl<Scalar>(converter);
	}

	[mutating]
	__generic <typename Scalar : __BuiltinArithmeticType, typename Target : IRWArray<Scalar>, typename Converter : IFunc<Scalar, Scalar>>
	void generate(inout Target target, uint N, const in Converter convert)
	{
		const uint S = sizeof(Scalar);
		const uint elem_per_roll = max(1, sizeof(Hash_t) / S);
		const uint rolls = (N + elem_per_roll - 1) / elem_per_roll;
		for(uint r = 0; r < rolls; ++r)
		{
			if(S == sizeof(Hash_t))
			{
				target[r] = convert(bit_cast<Scalar>(roll()));
			}
			else if(S > sizeof(Hash_t))
			{
				target[r] = convert(bit_cast<Scalar>(randomU64()));
			}
			else
			{
				const Hash_t chunk = roll();
				uint this_count = elem_per_roll;
				if((r == (rolls - 1)))
				{
					const uint remaining = N % rolls;
					this_count = remaining;
				}
				for(uint i = 0; i < this_count; ++i)
				{
					Hash_t token = (chunk >> (i * S)) & BitMask<Hash_t>(ConvertBuiltInTo<Hash_t>(S));
					target[r * elem_per_roll + i] = convert(reinterpret<Scalar>(token));
				}
			}
		}
	}

	[mutating]
	__generic <typename Scalar : __BuiltinArithmeticType, typename Target : IRWArray<Scalar>>
	void generate(inout Target target, uint N)
	{
		impl::RNGConverter<Scalar> converter = impl::RNGConverter<Scalar>();
		return this.generate<Scalar>(target, N, converter);
	}

	[mutating]
	__generic <typename Scalar : __BuiltinArithmeticType, typename Target : IRWArray<Scalar>>
	void generate(inout Target target, uint N, Scalar min, Scalar length)
	{
		impl::RNGConverter<Scalar> converter;
		converter.min = min;
		converter.length = length;
		return this.generate<Scalar>(target, N, converter);
	}


	[mutating]
	__generic <typename Scalar : __BuiltinArithmeticType, int N>
	Vector<Scalar, N> generate()
	{
		Vector<Scalar, N> res;
		this.generate<Scalar>(res, uint(N));
		return res;
	}

	[mutating]
	__generic <typename Scalar : __BuiltinArithmeticType, int N, typename Converter : IFunc<Scalar, Scalar>>
	Vector<Scalar, N> generate(const in Converter converter)
	{
		Vector<Scalar, N> res;
		this.generate<Scalar>(res, uint(N), converter);
		return res;
	}

	[mutating]
	__generic <typename Scalar : __BuiltinArithmeticType, int N>
	Vector<Scalar, N> generate(Scalar min, Scalar length)
	{
		Vector<Scalar, N> res;
		this.generate<Scalar>(res, uint(N), min, length);
		return res;
	}
};


typealias FastRNG_t = RandomNumberGenerator<fast_hash_t>;
typealias SlowRNG_t = RandomNumberGenerator<slow_hash_t>;
typealias RNG_t = RandomNumberGenerator<hash_t>;

