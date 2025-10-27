#pragma once

#include "Transforms.slang"

#include "Shared/AffineXForm.inl"


__generic <typename T : __BuiltinFloatingPointType, int N>
AffineXForm<T, N> mul(const in AffineXForm<T, N> l, const in AffineXForm<T, N> r)
{
#if 1
	return l * ResizeMatrix<N + 1, N + 1>(r);
#else
	const Matrix<T, N, N> Q = ExtractQBlock(l);
	return MakeAffineTransform(Q * ExtractQBlock(r), Q * ExtractTranslation(r) + ExtractTranslation(l));
#endif
}

__generic <typename T : __BuiltinFloatingPointType, int N>
AffineXForm<T, N> operator*(const in AffineXForm<T, N> l, const in AffineXForm<T, N> r)
{
	return mul(l, r);
}

__generic <typename Scalar : __BuiltinFloatingPointType, int N>
Matrix<Scalar, N, N> DirectionMatrix(const in AffineXForm<Scalar, N> xform)
{
	return DirectionMatrix(ExtractQBlock(xform));
}
