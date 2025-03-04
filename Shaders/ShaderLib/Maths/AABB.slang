#pragma once

#include <ShaderLib/common.slang>
#include <ShaderLib/Maths/NumericLimits.slang>

template <typename T : __BuiltinArithmeticType, int N>
struct AlignedAxisBoundingBox
{
	typealias This_t = AlignedAxisBoundingBox<T, N>;
	typealias vec = Vector<T, N>;

	vec _bottom;
	vec _top;

	[mutating]
	void reset()
	{
		_bottom = vec(NumericLimits<T>::infinity());
		_top = vec(-NumericLimits<T>::infinity());
	}

	__init()
	{
		reset();
	}

	__init(const in This_t o)
	{
		_bottom = o._bottom;
		_top = o._top;
	}

	vec bottom()
	{
		return _bottom;
	}

	vec top()
	{
		return _top;
	}

	vec diagonal()
	{
		return top() - bottom();
	}

	[mutating]
	void integrate(const in vec v)
	{
		_bottom = Min(_bottom, v);
		_top = Max(_top, v);
	}

	[mutating]
	void integrate(const in This_t o)
	{
		_bottom = Min(_bottom, o.bottom());
		_top = Max(_top, o.top());
	}

	This_t operator+(const in vec v)
	{
		This_t res = This_t(this);
		res.integrate(v);
		return res;
	}

	This_t operator+(const in This_t o)
	{
		This_t res = This_t(this);
		res.integrate(o);
		return res;
	}

	bool empty()
	{
		return _bottom[0] > _top[0];
	}

	bool containsStrict(const in vec p)
	{
		bool res = true;
		for(uint i = 0; i < N; ++i)
		{
			res &= (p[i] > _bottom[i]);
			res &= (p[i] < _top[i]);
		}
		return res;
	}

	bool containsRelaxed(const in vec p)
	{
		bool res = true;
		for(uint i = 0; i < N; ++i)
		{
			res &= (p[i] >= _bottom[i]);
			res &= (p[i] <= _top[i]);
		}
		return res;
	}

	This_t intersect(const in This_t o)
	{
		This_t res;
		res._bottom = Max(_bottom, o._bottom);
		res._top = Min(_top, o._top);
		return res;
	}

	bool operator==(const in This_t o)
	{
		return all(_bottom == o._bottom) && all(_top == o._top);
	}

	// 3D: Volume
	// 2D: Surface
	// 1D: Length
	T measureInside()
	{
		return Prod(diagonal());
	}

	// 3D: Surface
	// 2D: Perimeter
	T measureBorder()
	{
		const vec d = diagonal();
		T res = T(0);
		for(uint i = 0; i < N; ++i)
		{
			T s = T(1);
			for(uint j = 0; j < N; ++j)
			{
				if(i != j)
				{
					s *= d[j];
				}
			}
			res += s;
		}
		return res * T(2);
	}
};

template <typename T : __BuiltinArithmeticType, int N>
typealias AABB = AlignedAxisBoundingBox<T, N>;

template <typename T : __BuiltinArithmeticType>
typealias AABB2 = AABB<T, 2>;

template <typename T : __BuiltinArithmeticType>
typealias AABB3 = AABB<T, 3>;

template <int N>
typealias AABBf = AABB<float, N>;

typealias AABB2f = AABB2<float>;
typealias AABB3f = AABB3<float>;

