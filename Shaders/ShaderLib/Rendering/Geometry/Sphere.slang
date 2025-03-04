#pragma once

#include "Geometry.slang"
#include <ShaderLib/Rendering/Ray.slang>

template <typename T : __BuiltinArithmeticType, int N>
struct HyperSphere : IGeometry<T, N>
{
	Vector<T, N> center;
	T radius;

	AABB<T, N> getAABB()
	{
		AABB<T, N> res;
		res._bottom = center - radius;
		res._top = center + radius;
		return res;
	}
};

template <typename T : __BuiltinArithmeticType>
typealias Circle = HyperSphere<T, 2>;

template <typename T : __BuiltinFloatingPointType>
typealias Sphere = HyperSphere<T, 3>;

typealias Circlef = Circle<float>;
typealias Spheref = Sphere<float>;

