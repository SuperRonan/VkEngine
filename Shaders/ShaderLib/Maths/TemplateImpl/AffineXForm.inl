
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
constexpr AFFINE_XFORM_INL_XFormMatrix MakeAffineTransform(ConstRef(AFFINE_XFORM_INL_QBlock) Q, ConstRef(AFFINE_XFORM_INL_Vector) t)
{
	AFFINE_XFORM_INL_XFormMatrix res = AFFINE_XFORM_INL_XFormMatrix(Q);
	res[AFFINE_XFORM_INL_Dims] = t;
	return res;
}

AFFINE_XFORM_INL_TEMPLATE_DECL
constexpr AFFINE_XFORM_INL_Vector ExtractTranslation(ConstRef(AFFINE_XFORM_INL_XFormMatrix) m)
{
	return m[AFFINE_XFORM_INL_Dims];
}

AFFINE_XFORM_INL_TEMPLATE_DECL
constexpr AFFINE_XFORM_INL_QBlock ExtractQBlock(ConstRef(AFFINE_XFORM_INL_XFormMatrix) m)
{
	return AFFINE_XFORM_INL_QBlock(m);
}

AFFINE_XFORM_INL_TEMPLATE_DECL
constexpr AFFINE_XFORM_INL_XFormMatrix mul(ConstRef(AFFINE_XFORM_INL_XFormMatrix) l, ConstRef(AFFINE_XFORM_INL_XFormMatrix) r)
{
	const AFFINE_XFORM_INL_XFormMatrix res1 = l * AFFINE_XFORM_INL_FullMatrix(r);;
	const AFFINE_XFORM_INL_QBlock Q = ExtractQBlock(l);
	const AFFINE_XFORM_INL_XFormMatrix res2 = MakeAffineTransform(Q * ExtractQBlock(r), Q * ExtractTranslation(r) + ExtractTranslation(l));
	return res1;
}

// To check: It probablya assumes Q = S R
// AFFINE_XFORM_INL_TEMPLATE_DECL
// constexpr AFFINE_XFORM_INL_Vector ExtractScale(ConstRef(AFFINE_XFORM_INL_QBlock) Q)
// {
// 	AFFINE_XFORM_INL_QBlock res;
// 	for(uint i = 0; i < 3; ++i)
// 	{
// 		res[i] = nmspc_glm length(RS[i]);
// 	}
// 	return res;
// }

// AFFINE_XFORM_INL_TEMPLATE_DECL
// constexpr vec3R ExtractScale(ConstRef(mat4x3R) m)
// {
// 	return ExtractScale(ExtractRSBlock(m));
// }

AFFINE_XFORM_INL_TEMPLATE_DECL
constexpr AFFINE_XFORM_INL_Scalar ExtractUniformScaleAssumeQis_sR(ConstRef(AFFINE_XFORM_INL_QBlock) sR)
{
	// If Q is indeed a sR, the two results should be the same
	const AFFINE_XFORM_INL_Scalar res1 = nmspc_glm length(sR[0]); 
	const AFFINE_XFORM_INL_Scalar res2 = nmspc_glm determinant(sR);
	const AFFINE_XFORM_INL_Scalar res = res1;
	return res;
}

// AFFINE_XFORM_INL_TEMPLATE_DECL
// constexpr AFFINE_XFORM_INL_QBlock NormalizeRotationScale(ConstRef(AFFINE_XFORM_INL_QBlock) Q)
// {
// 	AFFINE_XFORM_INL_QBlock res;
// 	for(uint i = 0; i < Dim; ++i)
// 	{
// 		res[i] = nmspc_glm normalize(RS[i]);
// 	}
// 	return res;
// }

// AFFINE_XFORM_INL_TEMPLATE_DECL
// constexpr mat3R NormalizeRotationScale(ConstRef(mat3R) RS, ConstRef(vec3R) S)
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
constexpr AFFINE_XFORM_INL_Vector ExtractEulerAnglesFromRotationMatrix(ConstRef(AFFINE_XFORM_INL_QBlock) R)
{
#if TARGET_C_PLUS_PLUS
	if constexpr (AFFINE_XFORM_INL_Dims == 3)
#endif
	{
		// TO Check if correct
		const AFFINE_XFORM_INL_Scalar x = nmspc_glm atan(R[1][2], R[2][2]);
		const AFFINE_XFORM_INL_Scalar y = nmspc_glm atan(-R[0][2], nmspc_glm sqrt(sqr(R[1][2]) + sqr(R[2][2])));
		const AFFINE_XFORM_INL_Scalar z = nmspc_glm atan(R[0][1], R[0][0]);
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
constexpr AFFINE_XFORM_INL_Vector ExtractEulerAnglesAssumeUniformScale(ConstRef(AFFINE_XFORM_INL_QBlock) sR)
{
	return ExtractEulerAnglesFromRotationMatrix(sR / ExtractUniformScaleAssumeQis_sR(sR));
}

#endif

#if TARGET_C_PLUS_PLUS || (AFFINE_XFORM_INL_Dims == 2)

AFFINE_XFORM_INL_TEMPLATE_DECL
constexpr AFFINE_XFORM_INL_Scalar ExtractRotationAngle(ConstRef(AFFINE_XFORM_INL_QBlock) R)
{
#if TARGET_C_PLUS_PLUS
	if constexpr (AFFINE_XFORM_INL_Dims == 2)
#endif
	{
		// TO Check if correct
		AFFINE_XFORM_INL_Scalar res = nmspc_glm atan(R[0][1], R[0][0]);
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
constexpr AFFINE_XFORM_INL_Scalar ExtractRotationAngleAssumeUniformScale(ConstRef(AFFINE_XFORM_INL_QBlock) sR)
{
	return ExtractRotationAngle(sR / ExtractUniformScaleAssumeQis_sR(sR));
}

#endif





AFFINE_XFORM_INL_TEMPLATE_DECL
constexpr AFFINE_XFORM_INL_QBlock InverseRotationMatrix(ConstRef(AFFINE_XFORM_INL_QBlock) R)
{
	return transpose(R);
}

AFFINE_XFORM_INL_TEMPLATE_DECL
constexpr AFFINE_XFORM_INL_QBlock Inverse_sR(ConstRef(AFFINE_XFORM_INL_QBlock) sR)
{
	const AFFINE_XFORM_INL_Scalar s = ExtractUniformScaleAssumeQis_sR(sR);

	const AFFINE_XFORM_INL_QBlock res = nmspc_glm transpose(sR) / (s * s);

	return res;
}

AFFINE_XFORM_INL_TEMPLATE_DECL
constexpr AFFINE_XFORM_INL_QBlock InverseGenericQBlock(ConstRef(AFFINE_XFORM_INL_QBlock) Q)
{
	// Probably the fastest
	return nmspc_glm inverse(Q);
}

AFFINE_XFORM_INL_TEMPLATE_DECL
constexpr AFFINE_XFORM_INL_XFormMatrix InverseRigidTransformFast(ConstRef(AFFINE_XFORM_INL_XFormMatrix) m)
{
	const AFFINE_XFORM_INL_QBlock R = ExtractQBlock(m);
	const AFFINE_XFORM_INL_Vector T = ExtractTranslation(m);

	const AFFINE_XFORM_INL_QBlock inv_R = InverseRotationMatrix(R);
	const AFFINE_XFORM_INL_Vector inv_T = -(inv_R * T);
	return MakeAffineTransform(inv_R, inv_T);
}

AFFINE_XFORM_INL_TEMPLATE_DECL
constexpr AFFINE_XFORM_INL_XFormMatrix InverseRigidTransform(ConstRef(AFFINE_XFORM_INL_XFormMatrix) m)
{
	return InverseRigidTransformFast(m);
}

AFFINE_XFORM_INL_TEMPLATE_DECL
constexpr AFFINE_XFORM_INL_XFormMatrix InverseUniformSimilarTransform(ConstRef(AFFINE_XFORM_INL_XFormMatrix) m)
{
	const AFFINE_XFORM_INL_QBlock sR = ExtractQBlock(m);
	const AFFINE_XFORM_INL_Vector T = ExtractTranslation(m);

	const AFFINE_XFORM_INL_QBlock inv_sR = Inverse_sR(sR);
	const AFFINE_XFORM_INL_Vector inv_T = -(inv_sR * T);
	return MakeAffineTransform(inv_sR, inv_T);
}

AFFINE_XFORM_INL_TEMPLATE_DECL
constexpr AFFINE_XFORM_INL_XFormMatrix InverseAffineTransform(ConstRef(AFFINE_XFORM_INL_XFormMatrix) m)
{
	const AFFINE_XFORM_INL_QBlock Q = ExtractQBlock(m);
	const AFFINE_XFORM_INL_Vector T = ExtractTranslation(m);

	const AFFINE_XFORM_INL_QBlock inv_Q = InverseGenericQBlock(Q);
	const AFFINE_XFORM_INL_Vector inv_T = -(inv_Q * T);
	return MakeAffineTransform(inv_Q, inv_T);
}



#undef TARGET_C_PLUS_PLUS
