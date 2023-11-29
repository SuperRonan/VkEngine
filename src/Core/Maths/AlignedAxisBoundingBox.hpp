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

	protected:
		
		vecN _bottom = vecN(std::numeric_limits<Float>::max());
		vecN _top = vecN(std::numeric_limits<Float>::lowest());

	public:

		AlignedAxisBoundingBox() = default;

		constexpr const vecN & bottom()const
		{
			return _bottom;
		}

		constexpr const vecN& top()const
		{
			return _top;
		}

		vecN diagonal()const
		{
			return _top - _bottom;
		}

		// TODO simd version

		AlignedAxisBoundingBox& operator+=(vecN const& p)
		{
			_bottom = glm::min(_bottom, p);
			_top = glm::max(_top, p);
			return *this;
		}

		AlignedAxisBoundingBox& operator+=(AlignedAxisBoundingBox const& o)
		{
			_bottom = glm::min(_bottom, o._bottom);
			_top = glm::max(_top, o._top);
			return *this;
		}

		AlignedAxisBoundingBox operator+(vecN const& p) const
		{
			AlignedAxisBoundingBox res = *this;
			res += p;
			return res;
		}

		AlignedAxisBoundingBox operator+(AlignedAxisBoundingBox const& o) const
		{
			AlignedAxisBoundingBox res = *this;
			res += o;
			return res;
		}

		bool isInsideStrict(vecN const& p)const
		{
			bool res = true;
			for (int i = 0; i < N; ++i)
			{
				res &= (p[i] > bottom()[i]);
				res &= (p[i] < top()[i]);
			}
			return res;
		}

		bool isInsideRelaxed(vecN const& p)const
		{
			bool res = true;
			for (int i = 0; i < N; ++i)
			{
				res &= (p[i] >= bottom()[i]);
				res &= (p[i] <= top()[i]);
			}
			return res;
		}

		AlignedAxisBoundingBox unionWith(AlignedAxisBoundingBox const& o)const
		{
			return operator+(o);
		}

		AlignedAxisBoundingBox intersection(AlignedAxisBoundingBox const& o)const
		{
			AlignedAxisBoundingBox res;
			res._bottom = glm::max(_bottom, o._bottom);
			res._top = glm::min(_top, o._top);
			return res;
		}

		bool isInit()const
		{
			return _bottom != vecN(std::numeric_limits<Float>::max());
		}

		// 3D -> Volume
		// 2D -> Area
		// 1D -> Length
		Float dimsN()const
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
		Float dimsNMinus()const
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

		void clear()
		{
			_bottom = vecN(std::numeric_limits<Float>::max());
			_top = vecN(std::numeric_limits<Float>::lowest());
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