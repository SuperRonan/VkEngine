#pragma once

#include <ShaderLib/common.slang>

// follow the interface of C++'s std::numeric_limits

namespace std
{
	__generic <typename T : __BuiltinArithmeticType>
	struct numeric_limits
	{
		// TODO implement the rest of the interface


		// Returns the largest finite value
		static constexpr T max()
		{
			T res;
			if(IS_BUILTIN_SIGNED_INT(T))
			{
				if(T is i8)
				{
					res = T(0x7Fu);
				}
				else if (T is i16)
				{
					res = T(0x7FFFu);
				}
				else if (T is i32)
				{
					res = T(0x7FFF_FFFFu);
				}
				else if(T is i64)
				{
					res = bit_cast<T>(0x7FFF_FFFF_FFFF_FFFFull);
				}
			}
			else if(IS_BUILTIN_UNSIGNED_INT(T))
			{
				if(T is u8)
				{
					res = T(0xFFu);
				}
				else if (T is u16)
				{
					res = T(0xFFFFu);
				}
				else if (T is u32)
				{
					res = T(0xFFFF_FFFFu);
				}
				else if(T is u64)
				{
					res = bit_cast<T>(0xFFFF_FFFF_FFFF_FFFFull);
				}
			}
			else if (IS_BUILTIN_FLOAT(T))
			{
				if(T is f16)
				{
					res = bit_cast<T>(u16(0x7bffu));
				}
				else if(T is f32)
				{
					res = bit_cast<T>(0x7f7f_ffffu);
				}
				else if(T is f64)
				{
					res = bit_cast<T>(0x7FEF_FFFF_FFFF_FFFFull);
				}
			}
			return res;
		}

		// For integer: returns the minimum value (0 for uint, -max-1 for int)
		// For float: returns the smallest positive normal value
		static constexpr T min()
		{
			T res = T(0);
			if(IS_BUILTIN_SIGNED_INT(T))
			{
				if(T is i8)
				{
					res = T(0x80u);
				}
				else if (T is i16)
				{
					res = T(0x8000u);
				}
				else if (T is i32)
				{
					res = T(0x8000_0000u);
				}
				else if(T is i64)
				{
					res = bit_cast<T>(0x8000_0000_0000_0000ull);
				}
			}
			else if(IS_BUILTIN_UNSIGNED_INT(T))
			{
				res = T(0);
			}
			else if(IS_BUILTIN_FLOAT(T))
			{
				if(T is f16)
				{
					res = bit_cast<T>(u16(0x0400u));
				}
				else if(T is f32)
				{
					res = bit_cast<T>(0x0080_0000u);
				}
				else if(T is f64)
				{
					res = bit_cast<T>(0x0010_0000_0000_0000ull);
				}
			}
			return res;
		}

		// For integer: returns the same as min()
		// For float: returns -max()
		static constexpr T lowest()
		{
			T res;
			if(IS_BUILTIN_INTEGER(T))
			{
				res = this.min();
			}
			else if(IS_BUILTIN_FLOAT(T))
			{
				res = -this.max();
			}
			return res;
		}

		// Float: Returns the difference between one and the next representable value
		// Integral: 0
		static constexpr T epsilon()
		{
			T res = T(0);
			if(T is f16)
			{
				res = bit_cast<T>(u16(0x3c01u));
			}
			else if(T is f32)
			{
				res = bit_cast<T>(0x3f80_0001u);
			}
			else if(T is f64)
			{
				res = bit_cast<T>(0x3FF0_0000_0000_0001ull);
			}
			return res;
		}

		// static T round_error()
		// {

		// }

		// Float: Returns +infinity
		// Integral: 0
		static constexpr T infinity()
		{
			T res = T(0);
			if(T is f16)
			{
				res = bit_cast<T>(u16(0x7c00u));
			}
			else if(T is f32)
			{
				res = bit_cast<T>(0x7f80_0000u);
			}
			else if(T is f64)
			{
				res = bit_cast<T>(0x7FF0_0000_0000_0000ull);
			}
			return res;
		}

		// Returns infinity if available, max otherwise
		static constexpr T InfinityOrMax()
		{
			T res;
			if(IS_BUILTIN_FLOAT(T))
			{
				res = infinity();
			}
			else
			{
				res = max();
			}
			return res;
		}

		// Float: Returns a qNaN
		// Integral: 0
		static constexpr T quiet_NaN()
		{
			T res = T(0);
			if(T is f16)
			{
				res = bit_cast<T>(u16(0b0_11111_1_0000_0000_1u));
			}
			else if(T is f32)
			{
				res = bit_cast<T>(0xffc0_0001u);
			}
			else if(T is f64)
			{
				res = bit_cast<T>(0x7FF8_0000_0000_0001ull);
			}
			return res;
		}

		// Float: Returns a sNaN
		// Integral: 0
		static constexpr T signaling_NaN()
		{
			T res = T(0);
			if(T is f16)
			{
				res = bit_cast<T>(u16(0b0_11111_0_0000_0000_1u));
			}
			else if(T is f32)
			{
				res = bit_cast<T>(0xff80_0001u);
			}
			else if(T is f64)
			{
				res = bit_cast<T>(0x7FF0_0000_0000_0001ull);
			}
			return res;
		}

		// Returns the minimum positive sub-normal value (if T has denormalized numbers
		// Integral: 0
		static constexpr T denom_min()
		{
			T res = T(0);
			if(T is f16)
			{
				res = bit_cast<T>(u16(0x0001u));
			}
			else if(T is f32)
			{
				res = bit_cast<T>(u32(1u));
			}
			else if(T is f64)
			{
				res = bit_cast<T>(u64(1ull));
			}
			return res;
		}

		static constexpr uint GetMantissaBits()
		{
			uint res = 0;
			if(T is f16)
			{
				res = 10;
			}
			else if(T is f32)
			{
				res = 23;
			}
			else if(T is f64)
			{
				res = 52;
			}
			return res;
		}
	};
}

__generic <typename T : __BuiltinArithmeticType>
typealias NumericLimits = std::numeric_limits<T>;

