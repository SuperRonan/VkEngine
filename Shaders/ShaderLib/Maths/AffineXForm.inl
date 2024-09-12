
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

#ifndef AFFINE_XFORM_INL_TEMPLATE_DECL
#error "AFFINE_XFORM_INL_TEMPLATE_DECL not defined"
#endif

#ifndef AFFINE_XFORM_INL_cpp_constexpr
#error "AFFINE_XFORM_INL_cpp_constexpr not defined"
#endif

#ifndef AFFINE_XFORM_INL_CRef
#error "AFFINE_XFORM_INL_CRef not defined"
#endif

#ifndef AFFINE_XFORM_INL_nmspc
#error "AFFINE_XFORM_INL_nmspc not defined"
#endif

#ifndef AFFINE_XFORM_INL_Dims
#error "AFFINE_XFORM_INL_Dims not defined"
#endif

#ifndef AFFINE_XFORM_INL_Scalar
#error "AFFINE_XFORM_INL_Scalar not defined"
#endif

#ifndef AFFINE_XFORM_INL_QBlock
#error "AFFINE_XFORM_INL_QBlock not defined"
#endif

#ifndef AFFINE_XFORM_INL_FullMatrix
#error "AFFINE_XFORM_INL_FullMatrix not defined"
#endif

#ifndef AFFINE_XFORM_INL_XFormMatrix
#error "AFFINE_XFORM_INL_XFormMatrix not defined"
#endif

#ifndef AFFINE_XFORM_INL_Vector
#error "AFFINE_XFORM_INL_Vector not defined"
#endif 

#ifdef __cplusplus
#define TARGET_C_PLUS_PLUS 1
#else 
#define TARGET_C_PLUS_PLUS 0
#endif


AFFINE_XFORM_INL_TEMPLATE_DECL
AFFINE_XFORM_INL_cpp_constexpr AFFINE_XFORM_INL_XFormMatrix MakeAffineTransform(
	AFFINE_XFORM_INL_CRef(AFFINE_XFORM_INL_QBlock) Q, 
	AFFINE_XFORM_INL_CRef(AFFINE_XFORM_INL_Vector) t
	)
{
	AFFINE_XFORM_INL_XFormMatrix res = AFFINE_XFORM_INL_XFormMatrix(Q);
	res[AFFINE_XFORM_INL_Dims] = t;
	return res;
}

AFFINE_XFORM_INL_TEMPLATE_DECL
AFFINE_XFORM_INL_cpp_constexpr AFFINE_XFORM_INL_Vector ExtractTranslation(AFFINE_XFORM_INL_CRef(AFFINE_XFORM_INL_XFormMatrix) m)
{
	return m[AFFINE_XFORM_INL_Dims];
}

AFFINE_XFORM_INL_TEMPLATE_DECL
AFFINE_XFORM_INL_cpp_constexpr AFFINE_XFORM_INL_QBlock ExtractQBlock(AFFINE_XFORM_INL_CRef(AFFINE_XFORM_INL_XFormMatrix) m)
{
	return AFFINE_XFORM_INL_QBlock(m);
}

AFFINE_XFORM_INL_TEMPLATE_DECL
AFFINE_XFORM_INL_cpp_constexpr AFFINE_XFORM_INL_XFormMatrix mul(AFFINE_XFORM_INL_CRef(AFFINE_XFORM_INL_XFormMatrix) l, AFFINE_XFORM_INL_CRef(AFFINE_XFORM_INL_XFormMatrix) r)
{
	const AFFINE_XFORM_INL_XFormMatrix res1 = l * AFFINE_XFORM_INL_FullMatrix(r);;
	const AFFINE_XFORM_INL_QBlock Q = ExtractQBlock(l);
	const AFFINE_XFORM_INL_XFormMatrix res2 = MakeAffineTransform(Q * ExtractQBlock(r), Q * ExtractTranslation(r) + ExtractTranslation(l));
	return res1;
}

// To check: It probablya assumes Q = S R
// AFFINE_XFORM_INL_TEMPLATE_DECL
// AFFINE_XFORM_INL_cpp_constexpr AFFINE_XFORM_INL_Vector ExtractScale(AFFINE_XFORM_INL_CRef(AFFINE_XFORM_INL_QBlock) Q)
// {
// 	AFFINE_XFORM_INL_QBlock res;
// 	for(uint i = 0; i < 3; ++i)
// 	{
// 		res[i] = AFFINE_XFORM_INL_nmspc length(RS[i]);
// 	}
// 	return res;
// }

// AFFINE_XFORM_INL_TEMPLATE_DECL
// AFFINE_XFORM_INL_cpp_constexpr vec3R ExtractScale(AFFINE_XFORM_INL_CRef(mat4x3R) m)
// {
// 	return ExtractScale(ExtractRSBlock(m));
// }

AFFINE_XFORM_INL_TEMPLATE_DECL
AFFINE_XFORM_INL_cpp_constexpr AFFINE_XFORM_INL_Scalar ExtractUniformScaleAssumeQis_sR(AFFINE_XFORM_INL_CRef(AFFINE_XFORM_INL_QBlock) sR)
{
	// If Q is indeed a sR, the two results should be the same
	const AFFINE_XFORM_INL_Scalar res1 = AFFINE_XFORM_INL_nmspc length(sR[0]); 
	const AFFINE_XFORM_INL_Scalar res2 = AFFINE_XFORM_INL_nmspc determinant(sR);
	const AFFINE_XFORM_INL_Scalar res = res1;
	return res;
}

// AFFINE_XFORM_INL_TEMPLATE_DECL
// AFFINE_XFORM_INL_cpp_constexpr AFFINE_XFORM_INL_QBlock NormalizeRotationScale(AFFINE_XFORM_INL_CRef(AFFINE_XFORM_INL_QBlock) Q)
// {
// 	AFFINE_XFORM_INL_QBlock res;
// 	for(uint i = 0; i < Dim; ++i)
// 	{
// 		res[i] = AFFINE_XFORM_INL_nmspc normalize(RS[i]);
// 	}
// 	return res;
// }

// AFFINE_XFORM_INL_TEMPLATE_DECL
// AFFINE_XFORM_INL_cpp_constexpr mat3R NormalizeRotationScale(AFFINE_XFORM_INL_CRef(mat3R) RS, AFFINE_XFORM_INL_CRef(vec3R) S)
// {
// 	mat3R res;
// 	for(uint i = 0; i < 3; ++i)
// 	{
// 		res[i] = RS[i] / S[i];
// 	}
// 	return res;
// }

#if TARGET_C_PLUS_PLUS || (AFFINE_XFORM_INL_Dims == 3)

AFFINE_XFORM_INL_TEMPLATE_DECL
AFFINE_XFORM_INL_cpp_constexpr AFFINE_XFORM_INL_Vector ExtractEulerAnglesFromRotationMatrix(AFFINE_XFORM_INL_CRef(AFFINE_XFORM_INL_QBlock) R)
{
#if TARGET_C_PLUS_PLUS
	if constexpr (AFFINE_XFORM_INL_Dims == 3)
#endif
	{
		// TO Check if correct
		const AFFINE_XFORM_INL_Scalar x = AFFINE_XFORM_INL_nmspc atan(R[1][2], R[2][2]);
		const AFFINE_XFORM_INL_Scalar y = AFFINE_XFORM_INL_nmspc atan(-R[0][2], AFFINE_XFORM_INL_nmspc sqrt(AFFINE_XFORM_INL_nmspc sqr(R[1][2]) + AFFINE_XFORM_INL_nmspc sqr(R[2][2])));
		const AFFINE_XFORM_INL_Scalar z = AFFINE_XFORM_INL_nmspc atan(R[0][1], R[0][0]);
		return AFFINE_XFORM_INL_Vector(x, y, z);
	}
#if TARGET_C_PLUS_PLUS
	else
	{
		return {};
	}
#endif
}

AFFINE_XFORM_INL_TEMPLATE_DECL
AFFINE_XFORM_INL_cpp_constexpr AFFINE_XFORM_INL_Vector ExtractEulerAnglesAssumeUniformScale(AFFINE_XFORM_INL_CRef(AFFINE_XFORM_INL_QBlock) sR)
{
	return ExtractEulerAnglesFromRotationMatrix(sR / ExtractUniformScaleAssumeQis_sR(sR));
}

#endif

#if TARGET_C_PLUS_PLUS || (AFFINE_XFORM_INL_Dims == 2)

AFFINE_XFORM_INL_TEMPLATE_DECL
AFFINE_XFORM_INL_cpp_constexpr AFFINE_XFORM_INL_Scalar ExtractRotationAngle(AFFINE_XFORM_INL_CRef(AFFINE_XFORM_INL_QBlock) R)
{
#if TARGET_C_PLUS_PLUS
	if constexpr (AFFINE_XFORM_INL_Dims == 2)
#endif
	{
		// TO Check if correct
		AFFINE_XFORM_INL_Scalar res = AFFINE_XFORM_INL_nmspc atan(R[0][1], R[0][0]);
		return res;
	}
#if TARGET_C_PLUS_PLUS
	else
	{
		return AFFINE_XFORM_INL_Scalar(0);
	}
#endif
}

AFFINE_XFORM_INL_TEMPLATE_DECL
AFFINE_XFORM_INL_cpp_constexpr AFFINE_XFORM_INL_Scalar ExtractRotationAngleAssumeUniformScale(AFFINE_XFORM_INL_CRef(AFFINE_XFORM_INL_QBlock) sR)
{
	return ExtractRotationAngle(sR / ExtractUniformScaleAssumeQis_sR(sR));
}

#endif





AFFINE_XFORM_INL_TEMPLATE_DECL
AFFINE_XFORM_INL_cpp_constexpr AFFINE_XFORM_INL_QBlock InverseRotationMatrix(AFFINE_XFORM_INL_CRef(AFFINE_XFORM_INL_QBlock) R)
{
	return transpose(R);
}

AFFINE_XFORM_INL_TEMPLATE_DECL
AFFINE_XFORM_INL_cpp_constexpr AFFINE_XFORM_INL_QBlock Inverse_sR(AFFINE_XFORM_INL_CRef(AFFINE_XFORM_INL_QBlock) sR)
{
	const AFFINE_XFORM_INL_Scalar s = ExtractUniformScaleAssumeQis_sR(sR);

	const AFFINE_XFORM_INL_QBlock res = AFFINE_XFORM_INL_nmspc transpose(sR) / (s * s);

	return res;
}

AFFINE_XFORM_INL_TEMPLATE_DECL
AFFINE_XFORM_INL_cpp_constexpr AFFINE_XFORM_INL_QBlock InverseGenericQBlock(AFFINE_XFORM_INL_CRef(AFFINE_XFORM_INL_QBlock) Q)
{
	// Probably the fastest
	return AFFINE_XFORM_INL_nmspc inverse(Q);
}

AFFINE_XFORM_INL_TEMPLATE_DECL
AFFINE_XFORM_INL_cpp_constexpr AFFINE_XFORM_INL_XFormMatrix InverseRigidTransformFast(AFFINE_XFORM_INL_CRef(AFFINE_XFORM_INL_XFormMatrix) m)
{
	const AFFINE_XFORM_INL_QBlock R = ExtractQBlock(m);
	const AFFINE_XFORM_INL_Vector T = ExtractTranslation(m);

	const AFFINE_XFORM_INL_QBlock inv_R = InverseRotationMatrix(R);
	const AFFINE_XFORM_INL_Vector inv_T = -(inv_R * T);
	return MakeAffineTransform(inv_R, inv_T);
}

AFFINE_XFORM_INL_TEMPLATE_DECL
AFFINE_XFORM_INL_cpp_constexpr AFFINE_XFORM_INL_XFormMatrix InverseUniformSimilarTransform(AFFINE_XFORM_INL_CRef(AFFINE_XFORM_INL_XFormMatrix) m)
{
	const AFFINE_XFORM_INL_QBlock sR = ExtractQBlock(m);
	const AFFINE_XFORM_INL_Vector T = ExtractTranslation(m);

	const AFFINE_XFORM_INL_QBlock inv_sR = Inverse_sR(sR);
	const AFFINE_XFORM_INL_Vector inv_T = -(inv_sR * T);
	return MakeAffineTransform(inv_sR, inv_T);
}

AFFINE_XFORM_INL_TEMPLATE_DECL
AFFINE_XFORM_INL_cpp_constexpr AFFINE_XFORM_INL_XFormMatrix InverseAffineTransform(AFFINE_XFORM_INL_CRef(AFFINE_XFORM_INL_XFormMatrix) m)
{
	const AFFINE_XFORM_INL_QBlock Q = ExtractQBlock(m);
	const AFFINE_XFORM_INL_Vector T = ExtractTranslation(m);

	const AFFINE_XFORM_INL_QBlock inv_Q = InverseGenericQBlock(Q);
	const AFFINE_XFORM_INL_Vector inv_T = -(inv_Q * T);
	return MakeAffineTransform(inv_Q, inv_T);
}



#undef TARGET_C_PLUS_PLUS
