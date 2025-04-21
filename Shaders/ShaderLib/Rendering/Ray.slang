#pragma once

#include <ShaderLib/common.slang>
#include <ShaderLib/Maths/NumericLimits.slang>

template <typename T : __BuiltinArithmeticType, int N>
struct Ray
{
	Vector<T, N> origin;
	Vector<T, N> direction;

	__init() {}

	__init(Vector<T, N> origin, Vector<T, N> direction)
	{
		this.origin = origin;
		this.direction = direction;
	}

	Vector<T, N> at(T t)
	{
		return origin + t * direction;
	}
};

template <typename T : __BuiltinFloatingPointType>
extension Ray<T, 3>
{
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

struct RayRange<typename T : __BuiltinArithmeticType>
{
	Vector2<T> range;

	[mutating]
	void resetRange()
	{
		range.x = T(0);
		range.y = NumericLimits<T>::InfinityOrMax();
	}

	__init()
	{
		resetRange();
	}

	__init(Vector2<T> v)
	{
		range = v;
	}

	__init(T min, T max)
	{
		range = {min, max};
	}

	__init(const in RayDesc rd)
	{
		range = {
			ConvertBuiltInTo<T>(rd.TMin),
			ConvertBuiltInTo<T>(rd.TMax)
		};
	}

	Vector2<T> getRange()
	{
		return range;
	}

	property T min
	{
		get {return range.x;}
		set {range.x = newValue;}
	}

	property T max
	{
		get {return range.y;}
		set {range.y = newValue;}
	}
};

typealias RayRangef = RayRange<float>;
struct RayDifferentials<typename T : __BuiltinFloatingPointType, int N>
{
	typealias Jacobian = Matrix<T, N, 2>;
	Jacobian origin_jacobian = Jacobian::Zero();
	Jacobian direction_jacobian = Jacobian::Zero();

	RayDifferentials<T, N> transmit(T t, Vector2<T> t_diff, Vector<T, N> direction)
	{
		RayDifferentials<T, N> res;
		res.direction_jacobian = direction_jacobian;
		res.origin_jacobian = origin_jacobian + t * direction_jacobian + OuterProduct(direction, t_diff);
		return res;
	}
};

typealias RayDiff3<Float:__BuiltinFloatingPointType> = RayDifferentials<Float, 3>;
typealias RayDiff3f = RayDiff3<float>;

extension RayDesc
{
	[mutating]
	void init<T:__BuiltinFloatingPointType>(const in Ray3<T> ray, const in RayRange<T> range)
	{
		Origin = vec3(ray.origin);
		Direction = vec3(ray.direction);
		TMin = range.min.toFloat();
		TMax = range.max.toFloat();
	}

	__init<T:__BuiltinFloatingPointType>(const in Ray3<T> ray, const in RayRange<T> range = {})
	{
		init(ray, range);
	}

	Ray3f getRay()
	{
		Ray3f res;
		res.origin = Origin;
		res.direction = Direction;
		return res;
	}

	RayRangef getRange()
	{
		return RayRangef(TMin, TMax);
	}
};
