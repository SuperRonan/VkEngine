#pragma once

// Some vocabulary on linear transforms:
// This headers deals with Affine linear transforms 
// It is intended for 2D and 3D transforms, N will be the number of dimensions
// Linear transfroms can be expressed in mat(N+1)xN (N+1 colums x N rows)
// The matrix is structure as: [Q, t]
// A "Q" block (matNxN), and a translation vector "t"
// Special cases:
// A Rigid transform: Q = R, an orthonormal Rotation matrix, also called Euclidian transform
// A UniformSimilarity transfrom: Q = sR, s a scalar
// These two special cases can be inverted more easily 

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), uint N>
constexpr AffineXForm<Scalar, N> MakeAffineTransform(CONST_REF(Matrix<Scalar, N, N>) Q, CONST_REF(Vector<Scalar, N>) t)
{
	Matrix<Scalar, N, N + 1> res = Matrix<Scalar, N, N + 1>(Q);
	SetColumn(res, N, t);
	return res;
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), uint N>
constexpr AffineXForm<Scalar, N> TranslationMatrix(CONST_REF(Vector<Scalar, N>) t)
{
	AffineXForm<Scalar, N> res = AffineXForm<Scalar, N>(DiagonalMatrix<N>(Scalar(1)));
	SetColumn(res, N, t);
	return res;
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), uint N>
constexpr AffineXForm<Scalar, N> ScalingMatrix(CONST_REF(Vector<Scalar, N>) s)
{
	AffineXForm<Scalar, N> res = AffineXForm<Scalar, N>(DiagonalMatrix<N>(Scalar(0)));
	for (uint i = 0; i < N; ++i)
	{
		SetCoeficient(res, i, i, s[i]);
	}
	return res;
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), uint N>
constexpr Vector<Scalar, N> ExtractTranslation(CONST_REF(Matrix<Scalar, N, N + 1>) m)
{
	return GetColumn(m, N);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), uint N>
constexpr Matrix<Scalar, N, N> ExtractQBlock(CONST_REF(AffineXForm<Scalar, N>) m)
{
	return Matrix<Scalar, N, N>(m);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), uint N>
constexpr AffineXForm<Scalar, N> mul(CONST_REF(AffineXForm<Scalar, N>) l, CONST_REF(AffineXForm<Scalar, N>) r)
{
	const AffineXForm<Scalar, N> res1 = l * Matrix<Scalar, N, N>(r);
	const Matrix<Scalar, N, N> Q = ExtractQBlock(l);
	const AffineXForm<Scalar, N> res2 = MakeAffineTransform(Q * ExtractQBlock(r), Q * ExtractTranslation(r) + ExtractTranslation(l));
	return res1;
}

// To check: It probablya assumes Q = S R
// template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), uint N>
// constexpr Vector<Scalar, N> ExtractScale(CONST_REF(Matrix<Scalar, N, N>) Q)
// {
// 	Matrix<Scalar, N, N> res;
// 	for(uint i = 0; i < 3; ++i)
// 	{
// 		res[i] = nmspc_glm length(RS[i]);
// 	}
// 	return res;
// }

// template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), uint N>
// constexpr vec3R ExtractScale(CONST_REF(mat4x3R) m)
// {
// 	return ExtractScale(ExtractRSBlock(m));
// }

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), uint N>
constexpr Scalar ExtractUniformScaleAssumeQis_sR(CONST_REF(Matrix<Scalar, N, N>) sR)
{
	// If Q is indeed a sR, the two results should be the same
	const Scalar res1 = Length(GetColumn(sR, 0)); 
	const Scalar res2 = Determinant(sR);
	const Scalar res = res1;
	return res;
}

// template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), uint N>
// constexpr Matrix<Scalar, N, N> NormalizeRotationScale(CONST_REF(Matrix<Scalar, N, N>) Q)
// {
// 	Matrix<Scalar, N, N> res;
// 	for(uint i = 0; i < Dim; ++i)
// 	{
// 		res[i] = nmspc_glm normalize(RS[i]);
// 	}
// 	return res;
// }

// template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), uint N>
// constexpr mat3R NormalizeRotationScale(CONST_REF(mat3R) RS, CONST_REF(vec3R) S)
// {
// 	mat3R res;
// 	for(uint i = 0; i < 3; ++i)
// 	{
// 		res[i] = RS[i] / S[i];
// 	}
// 	return res;
// }

//#if TARGET_C_PLUS_PLUS || (AFFINE_XFORM_INL_Dims == 3)
//
//template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), uint N>
//constexpr Vector<Scalar, N> ExtractEulerAnglesFromRotationMatrix(CONST_REF(Matrix<Scalar, N, N>) R)
//{
//#if TARGET_C_PLUS_PLUS
//	if constexpr (AFFINE_XFORM_INL_Dims == 3)
//#endif
//	{
//		// TO Check if correct
//		const Scalar x = nmspc_glm atan(R[1][2], R[2][2]);
//		const Scalar y = nmspc_glm atan(-R[0][2], nmspc_glm sqrt(sqr(R[1][2]) + sqr(R[2][2])));
//		const Scalar z = nmspc_glm atan(R[0][1], R[0][0]);
//		return Vector<Scalar, N>(x, y, z);
//	}
//#if TARGET_C_PLUS_PLUS
//	else
//	{
//		return {};
//	}
//#endif
//}
//
//template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), uint N>
//constexpr Vector<Scalar, N> ExtractEulerAnglesAssumeUniformScale(CONST_REF(Matrix<Scalar, N, N>) sR)
//{
//	return ExtractEulerAnglesFromRotationMatrix(sR / ExtractUniformScaleAssumeQis_sR(sR));
//}
//
//#endif
//
//#if TARGET_C_PLUS_PLUS || (AFFINE_XFORM_INL_Dims == 2)
//
//template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), uint N>
//constexpr Scalar ExtractRotationAngle(CONST_REF(Matrix<Scalar, N, N>) R)
//{
//#if TARGET_C_PLUS_PLUS
//	if constexpr (AFFINE_XFORM_INL_Dims == 2)
//#endif
//	{
//		// TO Check if correct
//		Scalar res = nmspc_glm atan(R[0][1], R[0][0]);
//		return res;
//	}
//#if TARGET_C_PLUS_PLUS
//	else
//	{
//		return Scalar(0);
//	}
//#endif
//}
//
//template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), uint N>
//constexpr Scalar ExtractRotationAngleAssumeUniformScale(CONST_REF(Matrix<Scalar, N, N>) sR)
//{
//	return ExtractRotationAngle(sR / ExtractUniformScaleAssumeQis_sR(sR));
//}
//
//#endif





template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), uint N>
constexpr Matrix<Scalar, N, N> InverseRotationMatrix(CONST_REF(Matrix<Scalar, N, N>) R)
{
	return Transpose(R);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), uint N>
constexpr Matrix<Scalar, N, N> Inverse_sR(CONST_REF(Matrix<Scalar, N, N>) sR)
{
	const Scalar s = ExtractUniformScaleAssumeQis_sR(sR);

	const Matrix<Scalar, N, N> res = Transpose(sR) / (s * s);

	return res;
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), uint N>
constexpr Matrix<Scalar, N, N> InverseGenericQBlock(CONST_REF(Matrix<Scalar, N, N>) Q)
{
	// Probably the fastest
	return Inverse(Q);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), uint N>
constexpr AffineXForm<Scalar, N> InverseRigidTransformFast(CONST_REF(AffineXForm<Scalar, N>) m)
{
	const Matrix<Scalar, N, N> R = ExtractQBlock(m);
	const Vector<Scalar, N> T = ExtractTranslation(m);

	const Matrix<Scalar, N, N> inv_R = InverseRotationMatrix(R);
	const Vector<Scalar, N> inv_T = -(inv_R * T);
	return MakeAffineTransform(inv_R, inv_T);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), uint N>
constexpr AffineXForm<Scalar, N> InverseRigidTransform(CONST_REF(AffineXForm<Scalar, N>) m)
{
	return InverseRigidTransformFast(m);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), uint N>
constexpr AffineXForm<Scalar, N> InverseUniformSimilarTransform(CONST_REF(AffineXForm<Scalar, N>) m)
{
	const Matrix<Scalar, N, N> sR = ExtractQBlock(m);
	const Vector<Scalar, N> T = ExtractTranslation(m);

	const Matrix<Scalar, N, N> inv_sR = Inverse_sR(sR);
	const Vector<Scalar, N> inv_T = -(inv_sR * T);
	return MakeAffineTransform(inv_sR, inv_T);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), uint N>
constexpr AffineXForm<Scalar, N> InverseAffineTransform(CONST_REF(AffineXForm<Scalar, N>) m)
{
	const Matrix<Scalar, N, N> Q = ExtractQBlock(m);
	const Vector<Scalar, N> T = ExtractTranslation(m);

	const Matrix<Scalar, N, N> inv_Q = InverseGenericQBlock(Q);
	const Vector<Scalar, N> inv_T = -(inv_Q * T);
	return MakeAffineTransform(inv_Q, inv_T);
}



#undef TARGET_C_PLUS_PLUS
