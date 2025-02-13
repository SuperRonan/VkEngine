#pragma once

#include "Transforms.slang"

#include "Shared/AffineXForm.inl"


template <typename T : __BuiltinFloatingPointType, int N>
AffineXForm<T, N> mul(const in AffineXForm<T, N> l, const in AffineXForm<T, N> r)
{
#if 1
	return l * ResizeMatrix<N + 1, N + 1>(r);
#else
	const Matrix<T, N, N> Q = ExtractQBlock(l);
	return MakeAffineTransform(Q * ExtractQBlock(r), Q * ExtractTranslation(r) + ExtractTranslation(l));
#endif
}

template <typename T : __BuiltinFloatingPointType, int N>
AffineXForm<T, N> operator*(const in AffineXForm<T, N> l, const in AffineXForm<T, N> r)
{
	return mul(l, r);
}