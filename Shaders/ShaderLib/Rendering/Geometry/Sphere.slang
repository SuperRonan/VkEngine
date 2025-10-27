#pragma once

#include "Geometry.slang"
#include <ShaderLib/Rendering/Ray.slang>

__generic <typename T : __BuiltinArithmeticType, int N>
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

__generic <typename T : __BuiltinArithmeticType>
typealias Circle = HyperSphere<T, 2>;

__generic <typename T : __BuiltinFloatingPointType>
typealias Sphere = HyperSphere<T, 3>;

typealias Circlef = Circle<float>;
typealias Spheref = Sphere<float>;

