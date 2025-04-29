#pragma once

#include <ShaderLib/common.slang>
#include <ShaderLib/Rendering/Ray.slang>

__generic <T : __BuiltinFloatingPointType, int N>
Matrix<T, N, N> NormalizeDifferentials(const in Vector<T, N> vec)
{
	const T l2 = Length2(vec);
	Matrix<T, N, N> res = DiagonalMatrix<N, N>(l2) - OuterProduct(vec, vec);
	return rcp(pow(l2, T(1.5))) * res;
}

// The resulting vector should be treated as a row vector
__generic<T : __BuiltinFloatingPointType, int N, int V>
Vector<T, V> DotDifferentials(
	const in Vector<T, N> u,
	const in Matrix<T, N, V> Ju,
	const in Vector<T, N> v,
	const in Matrix<T, N, V> Jv)
{
	Vector<T, V> res = Transpose(Ju) * v + Transpose(Jv) * u;
	return res;
}

// dir  n    reflected
// ^    ^    ^
//  \   |   /
//   \  |  /
//    \ | /
//     \|/
//------------
// direction and normal face out of the surface
// Normal is assumed to be normalized
Vector<T, N> Reflect<T : __BuiltinFloatingPointType, int N>(const in Vector<T, N> direction, const in Vector<T, N> normal)
{
	return reflect(-direction, normal);
}

Vector<T, N> Refract<T : __BuiltinFloatingPointType, int N>(const in Vector<T, N> direction, const in Vector<T, N> normal, const in T eta)
{
	return refract(-direction, normal, eta);
}


__generic <T : __BuiltinFloatingPointType, int N, int V>
Matrix<T, N, V> ReflectionDifferentials(
	const in Vector<T, N> direction,
	const in Vector<T, N> normal,
	const in Matrix<T, N, V> Jdirection,
	const in Matrix<T, N, V> Jnormal)
{
	const Vector<T, V> Jd = DotDifferentials(direction, Jdirection, normal, Jnormal);
	Matrix<T, N, V> res = -Jdirection + T(2) * (
		Dot(direction, normal) * Jnormal +
		OuterProduct(normal, Jd)
	);	
	// The result might be reversed
	return res;
}


__generic <T : __BuiltinFloatingPointType, int N, int V>
Matrix<T, N, V> RefractionDifferentials(
	const in Vector<T, N> direction,
	const in Vector<T, N> normal,
	const in Matrix<T, N, V> Jdirection,
	const in Matrix<T, N, V> Jnormal,
	const in Vector<T, N> refracted,
	T eta,
	)
{
	// TODO check 
	const T ddn = Dot(-direction, normal);
	const T rdn = Dot(refracted, normal); 
	const T mu = eta * ddn - rdn;
	Vector<T, V> Jmu = (eta - (sqr(eta) * ddn) / (rdn)) * DotDifferentials(-direction, -Jdirection, normal, Jnormal);
	Matrix<T, N, V> res = -eta * Jdirection - (mu * Jnormal + OuterProduct(normal, Jmu));

	return res;
}



vec2 DifferentiateSurfaceIntersectionT(const in vec3 ray_dir, const in vec3 normal, float t, const in RayDiff3f ray_diffs)
{
	const float abs_cos_theta = Abs(Dot(normal, ray_dir));
	const vec2 res = (Transpose(ray_diffs.origin_jacobian + t * ray_diffs.direction_jacobian) * normal) / abs_cos_theta;
	return res;
}

__generic <T : __BuiltinFloatingPointType>
Vector2<T> ddxy(const in T t)
{
	return Vector2<T>(ddx(t), ddy(t));
}

__generic <T : __BuiltinFloatingPointType, int N>
Matrix<T, N , 2> ddxy(const in Vector<T, N> v)
{
	return MakeFromCols(ddx(v), ddy(v));
}

