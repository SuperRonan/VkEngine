#pragma once

#include "Types.hpp"
#include <numeric>
#include <limits>

namespace vkl
{
	template <int N, class Float>
	class AlignedAxisBoundingBox
	{
	public:
		
		using vecN = Vector<N, Float>;

		static constexpr vecN defaultBottom()
		{
			return vecN(std::numeric_limits<Float>::max());
		}

		static constexpr vecN defaultTop()
		{
			return vecN(std::numeric_limits<Float>::lowest());
		}

	protected:
		
		vecN _bottom = defaultBottom();
		vecN _top = defaultTop();

		constexpr bool invariant() const
		{
			bool res = empty();
			if (!res)
			{

			}
			return res;
		}

	public:

		constexpr AlignedAxisBoundingBox() = default;

		constexpr AlignedAxisBoundingBox(vecN const& bottom, vecN const& top):
			_bottom(bottom),
			_top(top)
		{}

		constexpr const vecN & bottom()const
		{
			return _bottom;
		}

		constexpr const vecN& top()const
		{
			return _top;
		}

		constexpr vecN diagonal()const
		{
			return _top - _bottom;
		}

		// TODO simd version

		constexpr AlignedAxisBoundingBox& operator+=(vecN const& p)
		{
			_bottom = glm::min(_bottom, p);
			_top = glm::max(_top, p);
			return *this;
		}

		constexpr AlignedAxisBoundingBox& operator+=(AlignedAxisBoundingBox const& o)
		{
			_bottom = glm::min(_bottom, o._bottom);
			_top = glm::max(_top, o._top);
			return *this;
		}

		constexpr AlignedAxisBoundingBox operator+(vecN const& p) const
		{
			AlignedAxisBoundingBox res = *this;
			res += p;
			return res;
		}

		constexpr AlignedAxisBoundingBox operator+(AlignedAxisBoundingBox const& o) const
		{
			AlignedAxisBoundingBox res = *this;
			res += o;
			return res;
		}

		constexpr bool isInsideStrict(vecN const& p)const
		{
			bool res = true;
			for (int i = 0; i < N; ++i)
			{
				res &= (p[i] > bottom()[i]);
				res &= (p[i] < top()[i]);
			}
			return res;
		}

		constexpr bool isInsideRelaxed(vecN const& p)const
		{
			bool res = true;
			for (int i = 0; i < N; ++i)
			{
				res &= (p[i] >= bottom()[i]);
				res &= (p[i] <= top()[i]);
			}
			return res;
		}

		constexpr AlignedAxisBoundingBox unionWith(AlignedAxisBoundingBox const& o)const
		{
			return operator+(o);
		}

		constexpr AlignedAxisBoundingBox intersection(AlignedAxisBoundingBox const& o)const
		{
			AlignedAxisBoundingBox res;
			res._bottom = glm::max(_bottom, o._bottom);
			res._top = glm::min(_top, o._top);
			return res;
		}

		constexpr bool isInit()const
		{
			return _bottom != vecN(std::numeric_limits<Float>::max());
		}

		// 3D -> Volume
		// 2D -> Area
		// 1D -> Length
		constexpr Float measureInside()const
		{
			Float res = Float(1);
			const vecN d = diagonal();
			for (int i = 0; i < N; ++i)
			{
				res *= d[i];
			}
			return res;
		}

		// 3D -> Surface Area
		// 2D -> Perimeter
		constexpr Float measureBorder()const
		{
			const vecN d = diagonal();
			Float res = 0;
			for (int i = 0; i < N; ++i)
			{
				Float s = Float(1);
				for (int j = 0; j < N; ++j)
				{
					if (i != j)
					{
						s *= d[j];
					}
				}
				res += s;
			}
			return res * Float(2);
		}

		constexpr void clear()
		{
			_bottom = defaultBottom();
			_top = defaultTop();
		}

		constexpr void reset()
		{
			clear();
		}

		constexpr bool empty()
		{
			return _bottom == defaultBottom() && _top == defaultTop();
		}

		//template <int M>
		//Float dims()const
		//{
		//	if constexpr (M == N)
		//	{
		//		return dimsN();
		//	}
		//	else if constexpr (M == (N - 1))
		//	{
		//		return dimsNMinus();
		//	}
		//	else
		//	{
		//		static_assert(false);
		//	}
		//}
	};

	template <int N, class Float>
	using AABB = AlignedAxisBoundingBox<N, Float>;

	template <int N>
	using AABBf = AABB<N, float>;

	using AABB3f = AABBf<3>;
}