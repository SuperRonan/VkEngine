#pragma once

#include <ShaderLib/common.slang>

#include <ShaderLib/Maths/AABB.slang>

struct SurfaceGeometryDifferentials<int N>
{
	Matrixf<2, N> Juv = Matrixf<2, N>::Zero();
	Matrixf<3, N> Jnormal = Matrixf<3, N>::Zero();
	Matrixf<3, N> Jposition = Matrixf<3, N>::Zero();

	__init() {}

	
};

__generic <int N, int M>
SurfaceGeometryDifferentials<M> operator*(const in SurfaceGeometryDifferentials<N> lhs,  const in Matrixf<N, M> xform)
{
	SurfaceGeometryDifferentials<M> res;
	res.Juv = lhs.Juv * xform;
	res.Jnormal = lhs.Jnormal * xform;
	res.Jposition = lhs.Jposition * xform;
	return res;
}

typealias SurfaceGeometryDifferentials2 = SurfaceGeometryDifferentials<2>;

__generic <typename T : __BuiltinArithmeticType, int N>
interface IGeometry
{

	AlignedAxisBoundingBox<T, N> getAABB();


}

