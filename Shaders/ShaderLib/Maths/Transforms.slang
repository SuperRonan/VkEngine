#pragma once

#include <ShaderLib/common.slang>

template <typename Scalar, int N>
typealias AffineXForm = Matrix<Scalar, N, N+1>;

template <typename Scalar>
typealias AffineXForm3D = AffineXForm<Scalar, 3>;

typealias AffineXForm3Df = AffineXForm3D<float>;

template <typename Scalar : __BuiltinArithmeticType, int N>
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

template <typename Scalar : __BuiltinFloatingPointType, int N>
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


#include "Shared/Common.inl"
