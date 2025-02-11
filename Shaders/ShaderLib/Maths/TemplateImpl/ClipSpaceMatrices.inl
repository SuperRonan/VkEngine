
#ifndef CLIP_SPACE_INL_TEMPLATE_DECL
#error "CLIP_SPACE_INL_TEMPLATE_DECL not defined"
#endif

#ifndef CLIP_SPACE_INL_Scalar
#error "CLIP_SPACE_INL_Scalar not defined"
#endif

#ifndef CLIP_SPACE_INL_Vector
#error "CLIP_SPACE_INL_Vector not defined"
#endif

#ifndef CLIP_SPACE_INL_Matrix
#error "Matrix not defined"
#endif

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Matrix(4) PerspectiveProjExplicit(ConstRef(CLIP_SPACE_INL_Vector(4)) coefs)
{
	CLIP_SPACE_INL_Matrix(4) res = CLIP_SPACE_INL_Matrix(4)(0);
	res[0][0] = coefs.x;
	res[1][1] = coefs.y;
	res[2][2] = coefs.z;
	res[2][3] = 1.0f;
	res[3][2] = coefs.w;
	return res;
}

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Vector(4) ExtractPerspectiveProjCoefs(ConstRef(CLIP_SPACE_INL_Matrix(4)) m)
{
	return CLIP_SPACE_INL_Vector(4)(
		m[0][0],
		m[1][1],
		m[2][2],
		m[3][2]
	);
}

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Vector(4) GetPerspectiveProjCoefsFromInvTanInvAspect(CLIP_SPACE_INL_Scalar inv_tan_half_fov, CLIP_SPACE_INL_Scalar inv_aspect, CLIP_SPACE_INL_Vector(2) z_range)
{
	CLIP_SPACE_INL_Vector(4) res;
	res.x = inv_tan_half_fov * inv_aspect;
	res.y = inv_tan_half_fov;
	res.z = z_range.y / (z_range.y - z_range.x);
	res.w = -z_range.x * res.z;
	return res;
}

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Matrix(4) PerspectiveProjFromInvTanInvAspect(CLIP_SPACE_INL_Scalar inv_tan_half_fov, CLIP_SPACE_INL_Scalar inv_aspect, CLIP_SPACE_INL_Vector(2) z_range)
{
	return PerspectiveProjExplicit(GetPerspectiveProjCoefsFromInvTanInvAspect(inv_tan_half_fov, inv_aspect, z_range));
}

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Matrix(4) PerspectiveProjFromTan(CLIP_SPACE_INL_Scalar tan_half_fov, CLIP_SPACE_INL_Scalar aspect, CLIP_SPACE_INL_Vector(2) z_range)
{
	return PerspectiveProjFromInvTanInvAspect(rcp(tan_half_fov), rcp(aspect), z_range);
}

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Matrix(4) PerspectiveProjFromFOV(CLIP_SPACE_INL_Scalar fov, CLIP_SPACE_INL_Scalar aspect, CLIP_SPACE_INL_Vector(2) z_range)
{
	return PerspectiveProjFromTan(TanHalfFOV(fov), aspect, z_range);
}

// Works in both directions
CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Vector(4) InversePerspectiveProjCoefs(CLIP_SPACE_INL_Vector(4) coefs)
{
	CLIP_SPACE_INL_Vector(4) res;
	res.x = rcp(coefs.x);
	res.y = rcp(coefs.y);
	res.w = rcp(coefs.w);
	res.z = - coefs.z * res.w;
	return res;
}

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Matrix(4) InversePerspectiveProjExplicit(CLIP_SPACE_INL_Vector(4) coefs)
{
	CLIP_SPACE_INL_Matrix(4) res = CLIP_SPACE_INL_Matrix(4)(0);
	res[0][0] = coefs.x;
	res[1][1] = coefs.y;
	res[3][2] = 1.0f;
	res[2][3] = coefs.w;
	res[3][3] = coefs.z;
	return res;
}

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Vector(4) GetInversePerspectiveProjCoefsFromTan(CLIP_SPACE_INL_Scalar tan_half_fov, CLIP_SPACE_INL_Scalar aspect, CLIP_SPACE_INL_Vector(2) z_range)
{
	CLIP_SPACE_INL_Vector(4) res;
	res.x = tan_half_fov * aspect;
	res.y = tan_half_fov;
	const CLIP_SPACE_INL_Scalar z = z_range.y / (z_range.y - z_range.x);
	const CLIP_SPACE_INL_Scalar w = -z_range.x * z;
	res.w = rcp(w);
	res.z = rcp(z_range.x);
	return res;
}

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Matrix(4) InversePerspectiveProjFromTan(CLIP_SPACE_INL_Scalar tan_half_fov, CLIP_SPACE_INL_Scalar aspect, CLIP_SPACE_INL_Vector(2) z_range)
{
	return InversePerspectiveProjExplicit(GetInversePerspectiveProjCoefsFromTan(tan_half_fov, aspect, z_range));
}

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Matrix(4) InversePerspectiveProjFromFOV(CLIP_SPACE_INL_Scalar fov, CLIP_SPACE_INL_Scalar aspect, CLIP_SPACE_INL_Vector(2) z_range)
{
	return InversePerspectiveProjFromTan(TanHalfFOV(fov), aspect, z_range);
}

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Vector(3) InverseInfinitePerspectiveProjCoefs(CLIP_SPACE_INL_Vector(3) coefs)
{
	CLIP_SPACE_INL_Vector(3) res = rcp(coefs);
	return res;
}

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Matrix(4) InfinitePerspectiveProjExplicit(CLIP_SPACE_INL_Vector(3) coefs)
{
	CLIP_SPACE_INL_Matrix(4) res = CLIP_SPACE_INL_Matrix(4)(0);
	res[0][0] = coefs.x;
	res[1][1] = coefs.y;
	res[2][2] = 1;
	res[3][2] = coefs.z;
	res[2][3] = 1;
	return res;
}

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Matrix(4) InverseInfinitePerspectiveProjExplicit(CLIP_SPACE_INL_Vector(3) coefs)
{
	CLIP_SPACE_INL_Matrix(4) res = CLIP_SPACE_INL_Matrix(4)(0);
	res[0][0] = coefs.x;
	res[1][1] = coefs.y;
	res[3][2] = 1;
	res[2][3] = coefs.z;
	res[3][3] = -coefs.z;
	return res;
}

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Vector(3) GetInfinitePerspectiveProjCoefsFromInvTanInvAspect(CLIP_SPACE_INL_Scalar inv_tan_half_fov, CLIP_SPACE_INL_Scalar inv_aspect, CLIP_SPACE_INL_Scalar z_near)
{
	CLIP_SPACE_INL_Vector(3) coefs;
	coefs.x = inv_tan_half_fov * inv_aspect;
	coefs.y = inv_tan_half_fov;
	coefs.z = - CLIP_SPACE_INL_Scalar(2) * z_near;
	return coefs;
}

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Vector(3) GetInfinitePerspectiveProjCoefsFromTan(CLIP_SPACE_INL_Scalar tan_half_fov, CLIP_SPACE_INL_Scalar aspect, CLIP_SPACE_INL_Scalar z_near)
{
	return GetInfinitePerspectiveProjCoefsFromInvTanInvAspect(rcp(tan_half_fov), rcp(aspect), z_near);
}

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Matrix(4) InfinitePerspectiveProjFromInvTanInvAspect(CLIP_SPACE_INL_Scalar inv_tan_half_fov, CLIP_SPACE_INL_Scalar inv_aspect, CLIP_SPACE_INL_Scalar z_near)
{
	return InfinitePerspectiveProjExplicit(GetInfinitePerspectiveProjCoefsFromInvTanInvAspect(inv_tan_half_fov, inv_aspect, z_near));
}

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Matrix(4) InfinitePerspectiveProjFromTan(CLIP_SPACE_INL_Scalar tan_half_fov, CLIP_SPACE_INL_Scalar aspect, CLIP_SPACE_INL_Scalar z_near)
{
	return InfinitePerspectiveProjExplicit(GetInfinitePerspectiveProjCoefsFromTan(tan_half_fov, aspect, z_near));
}

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Matrix(4) InfinitePerspectiveProjFromFOV(CLIP_SPACE_INL_Scalar fov, CLIP_SPACE_INL_Scalar aspect, CLIP_SPACE_INL_Scalar z_near)
{
	const CLIP_SPACE_INL_Scalar tan_half_fov = TanHalfFOV(fov);
	return InfinitePerspectiveProjFromTan(tan_half_fov, aspect, z_near);
}

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Vector(3) GetInverseInfinitePerspectiveProjCoefsFromTanInvZnear(CLIP_SPACE_INL_Scalar tan_half_fov, CLIP_SPACE_INL_Scalar aspect, CLIP_SPACE_INL_Scalar inv_z_near)
{
	CLIP_SPACE_INL_Vector(3) coefs;
	coefs.x = tan_half_fov * aspect;
	coefs.y = aspect;
	coefs.z = - inv_z_near / CLIP_SPACE_INL_Scalar(2);
	return coefs;
}

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Matrix(4) InverseInfinitePerspectiveProjFromTanInvZnear(CLIP_SPACE_INL_Scalar tan_half_fov, CLIP_SPACE_INL_Scalar aspect, CLIP_SPACE_INL_Scalar inv_z_near)
{
	return InverseInfinitePerspectiveProjExplicit(GetInverseInfinitePerspectiveProjCoefsFromTanInvZnear(tan_half_fov, aspect, inv_z_near));
}

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Matrix(4) InverseInfinitePerspectiveProjFromTan(CLIP_SPACE_INL_Scalar tan_half_fov, CLIP_SPACE_INL_Scalar aspect, CLIP_SPACE_INL_Scalar z_near)
{
	return InverseInfinitePerspectiveProjExplicit(GetInverseInfinitePerspectiveProjCoefsFromTanInvZnear(tan_half_fov, aspect, rcp(z_near)));
}

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Matrix(4) InverseInfinitePerspectiveProjFromFOV(CLIP_SPACE_INL_Scalar fov, CLIP_SPACE_INL_Scalar aspect, CLIP_SPACE_INL_Scalar z_near)
{
	return InverseInfinitePerspectiveProjFromTan(TanHalfFOV(fov), aspect, z_near);
}


// lbn: left, bottom, near
// rtf: right, top, far
CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Matrix(4) OrthoProj(CLIP_SPACE_INL_Vector(3) lbn, CLIP_SPACE_INL_Vector(3) rtf)
{
	const CLIP_SPACE_INL_Vector(3) d = rtf - lbn;
	const CLIP_SPACE_INL_Vector(3) s = rtf + lbn;
	const CLIP_SPACE_INL_Vector(3) factors = CLIP_SPACE_INL_Vector(3)(2, 2, 1);
	const CLIP_SPACE_INL_Vector(3) diag = factors / d;
	const CLIP_SPACE_INL_Matrix(3) Q = DiagonalMatrix(diag);
	const CLIP_SPACE_INL_Vector(3) T = CLIP_SPACE_INL_Vector(3)(-s.x / d.x, -s.y / d.y, -lbn.z / d.z);
	const CLIP_SPACE_INL_Matrix(4) res = CLIP_SPACE_INL_Matrix(4)(MakeAffineTransform(Q, T));
	return res;
}

CLIP_SPACE_INL_TEMPLATE_DECL
constexpr CLIP_SPACE_INL_Matrix(4) InverseOrthoProj(CLIP_SPACE_INL_Vector(3) lbn, CLIP_SPACE_INL_Vector(3) rtf)
{
	const CLIP_SPACE_INL_Vector(3) d = rtf - lbn;
	const CLIP_SPACE_INL_Vector(3) s = rtf + lbn;
	const CLIP_SPACE_INL_Vector(3) factors = CLIP_SPACE_INL_Vector(3)(2, 2, 1);
	const CLIP_SPACE_INL_Vector(3) diag = d / factors;
	const CLIP_SPACE_INL_Matrix(3) Q = DiagonalMatrix(diag);
	const CLIP_SPACE_INL_Vector(3) T = CLIP_SPACE_INL_Vector(3)(s.x, s.y, lbn.z) / factors;
	const CLIP_SPACE_INL_Matrix(4) res = CLIP_SPACE_INL_Matrix(4)(MakeAffineTransform(Q, T));
	return res;
}

