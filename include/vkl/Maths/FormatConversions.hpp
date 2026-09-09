#pragma once

#include <that/img/FormatConversion.hpp>
#include "Types.hpp"

namespace vkl
{
	template <std::integral I, that::concepts::FloatingPoint F, int N>
	constexpr Vector<I, N> PackNorm(Vector<F, N> const& v)
	{
		// that::ConvertFloatToNorm clamps then converts
		// TODO Make faster unclamped version (at the (reasonable) risk of overflow)
		return v.unaryExpr(std::ref(that::template ConvertFloatToNorm<I, F>));
	}

	template <that::concepts::FloatingPoint F, std::integral I, int N>
	constexpr Vector<F, N> UnPackNorm(Vector<I, N> const& v)
	{
		return v.unaryExpr(std::ref(that::template ConvertNormToFloat<F, I>));
	}
}