#pragma once

#include <ShaderLib/common.slang>

#include <ShaderLib/Maths/AABB.slang>

struct SurfaceGeometryDifferentials<int N>
{
	Matrixf<2, N> uv_jacobians = Matrixf<2, N>::Zero();
	Matrixf<3, N> normal_jacobian = Matrixf<3, N>::Zero();
	Matrixf<3, N> position_jacobian = Matrixf<3, N>::Zero();

	__init() {}
};

typealias SurfaceGeometryDifferentials2 = SurfaceGeometryDifferentials<2>;

template <typename T : __BuiltinArithmeticType, int N>
interface IGeometry
{

	AlignedAxisBoundingBox<T, N> getAABB();


}

