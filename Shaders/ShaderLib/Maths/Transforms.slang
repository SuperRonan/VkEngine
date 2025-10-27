#pragma once

#include <ShaderLib/common.slang>

__generic <typename Scalar, int N>
typealias AffineXForm = Matrix<Scalar, N, N+1>;

__generic <typename Scalar>
typealias AffineXForm3D = AffineXForm<Scalar, 3>;

typealias AffineXForm3Df = AffineXForm3D<float>;

__generic <typename Scalar : __BuiltinArithmeticType, int N>
Vector<Scalar, N + 1> Homogeneous(const in Vector<Scalar, N> vec, Scalar h = Scalar(1))
{
	Vector<Scalar, N + 1> res;
	for(uint i = 0; i < N; ++i)
	{
		res[i] = vec[i];
	}
	res[N] = h;
	return res;
}

__generic <typename Scalar : __BuiltinFloatingPointType, int N>
Vector<Scalar, N - 1> HomogeneousNormalize(const in Vector<Scalar, N> vec)
{
	Vector<Scalar, N - 1> res;
	for(uint i = 0; i < (N-1); ++i)
	{
		res[i] = vec[i];
	}
	if(vec[N-1] != Scalar(0))
	{
		res = res * rcp(vec[N-1]);
	}
	return res;
}

// The function is generalized to a N-th dimensional simplex
__generic <typename Scalar : __BuiltinArithmeticType, int N>
Vector<Scalar, N + 1> UVToBarycentric(const in Vector<Scalar, N> v)
{
	Vector<Scalar, N + 1> res;
	res[0] = Scalar(1) - Sum(v);
	for(uint i = 0; i < N; ++i)
	{
		res[i + 1] = v[i];
	}
	return res;
}

__generic <typename Scalar : __BuiltinFloatingPointType, int N>
Matrix<Scalar, N, N> DirectionMatrix(const in Matrix<Scalar, N, N> m)
{
	return Transpose(Inverse(m));
}

#include "Shared/Common.inl"
