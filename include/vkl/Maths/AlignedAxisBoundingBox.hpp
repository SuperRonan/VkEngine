#pragma once

#include "Types.hpp"
#include "Transforms.hpp"
#include <numeric>
#include <limits>

namespace vkl
{
	template <uint N, class Float>
	class BoundingSphere
	{
	public:
		using vecN = Vector<Float, N>;

	protected:

		vecN _center;
		Float _radius;

	public:

		BoundingSphere(vecN center, Float radius):
			_center(center),
			_radius(radius)
		{}

		constexpr auto center() const
		{
			return _center;
		}

		constexpr auto radius() const
		{
			return _radius;
		}
	};

	template <uint N, class Float>
	class AlignedAxisBoundingBox
	{
	public:
		
		using vecN = Vector<Float, N>;

		static constexpr vecN defaultBottom()
		{
			return MakeUniformVector<N>(std::numeric_limits<Float>::max());
		}

		static constexpr vecN defaultTop()
		{
			return MakeUniformVector<N>(std::numeric_limits<Float>::lowest());
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

		constexpr vecN center()const
		{
			return Float(0.5) * _bottom + Float(0.5) * _top;
			//return (_bottom + _top) * Float(0.5);
		}

		// TODO simd version

		constexpr AlignedAxisBoundingBox& operator+=(vecN const& p)
		{
			_bottom = _bottom.cwiseMin(p);
			_top = _top.cwiseMax(p);
			return *this;
		}

		constexpr AlignedAxisBoundingBox& operator+=(AlignedAxisBoundingBox const& o)
		{
			_bottom = _bottom.cwiseMin(o._bottom);
			_top = _top.cwiseMax(o._top);
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
			for (uint i = 0; i < N; ++i)
			{
				res &= (p[i] > bottom()[i]);
				res &= (p[i] < top()[i]);
			}
			return res;
		}

		constexpr bool isInsideRelaxed(vecN const& p)const
		{
			bool res = true;
			for (uint i = 0; i < N; ++i)
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
			res._bottom = _bottom.cwiseMax(o._bottom);
			res._top = _top.cwiseMin(o._top);
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
			for (uint i = 0; i < N; ++i)
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
			for (uint i = 0; i < N; ++i)
			{
				Float s = Float(1);
				for (uint j = 0; j < N; ++j)
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

		constexpr bool empty() const
		{
			return (_bottom == defaultBottom() && _top == defaultTop());
		}

		constexpr void getContainingAABB(AffineXForm<Float, N> const& xform, AlignedAxisBoundingBox & res) const
		{
			const constexpr uint M = uint(1) << N;
			for (uint i = 0; i < M; ++i)
			{
				vecN p;
				for (uint j = 0; j < N; ++j)
				{
					bool b = (i & (1 << j)) != 0;
					p[j] = b ? _top[j] : _bottom[j];
				}
				res += (xform * p.homogeneous());
			}
		}

		constexpr AlignedAxisBoundingBox getContainingAABB(AffineXForm<Float, N> const& xform) const
		{
			AlignedAxisBoundingBox res = {};
			getContainingAABB(xform, res);
			return res;
		}

		constexpr BoundingSphere<N, Float> getContainingSphere() const
		{
			return BoundingSphere<N, Float>(
				center(),
				Length(diagonal()) * Float(0.5)
			);
		}

		//template <uint M>
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

	template <uint N, class Float>
	using AABB = AlignedAxisBoundingBox<N, Float>;

	template <uint N>
	using AABBf = AABB<N, float>;

	using AABB3f = AABBf<3>;
}