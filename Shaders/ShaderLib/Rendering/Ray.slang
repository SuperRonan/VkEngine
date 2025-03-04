#pragma once

#include <ShaderLib/common.slang>
#include <ShaderLib/Maths/NumericLimits.slang>

template <typename T : __BuiltinArithmeticType, int N>
struct Ray
{
	Vector<T, N> origin;
	Vector<T, N> direction;

	Vector<T, N> sample(T t)
	{
		return origin + t * direction;
	}
};

template <typename T : __BuiltinFloatingPointType>
extension Ray<T, 3>
{
	RayDesc getRayDesc()
	{
		RayDesc res;
		res.Origin = vec3(origin);
		res.Direction = vec3(direction);
		res.TMin = 0;
		res.TMax = NumericLimits<float>::max();
		return res;
	}

	__init(const in RayDesc rd)
	{
		origin = Vector3<T>(rd.Origin);
		direction = Vector3<T>(rd.Direction);
	}

};


template <T : __BuiltinArithmeticType>
typealias Ray2 = Ray<T, 2>;

template <T : __BuiltinArithmeticType>
typealias Ray3 = Ray<T, 3>;

template <int N>
typealias Rayf = Ray<float, N>;

typealias Ray2f = Ray2<float>;
typealias Ray3f = Ray3<float>;

template <typename T : __BuiltinArithmeticType, int N>
struct RayAndRange : Ray<T, N>
{
	Vector2<T> range;

	[mutating]
	void resetRange()
	{
		range.x = T(0);
		range.y = NumericLimits<T>::max();
	}

	__init(const in Ray<T, N> r)
	{
		origin = r.origin;
		direction = r.direction;
		resetRange();
	}

	Vector2<T> getRange()
	{
		return range;
	}

	T getMin()
	{
		return range.x;
	}

	T getMax()
	{
		return range.y;
	}
};

template <typename T : __BuiltinFloatingPointType>
extension RayAndRange<T, 3>
{
	[OverloadRank(15)]
	RayDesc getRayDesc()
	{
		RayDesc res;
		res.Origin = vec3(origin);
		res.Direction = vec3(direction);
		res.TMin = getMin().toFloat();
		res.TMax = getMax().toFloat();
		return res;
	}

	__init(const in RayDesc rd)
	{
		origin = Vector3<T>(rd.Origin);
		direction = Vector3<T>(rd.Direction);
		range = {T(rd.TMin), T(rd.TMax)};
	}
}

template <T : __BuiltinArithmeticType>
typealias Ray2AndRange = RayAndRange<T, 2>;

template <T : __BuiltinArithmeticType>
typealias Ray3AndRange = RayAndRange<T, 3>;

template <int N>
typealias RayfAndRange = RayAndRange<float, N>;

typealias Ray2fAndRange = Ray2AndRange<float>;
typealias Ray3fAndRange = Ray3AndRange<float>;
