#pragma once

#include <ShaderLib/interop_slang_cpp>

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr AffineXForm3D<Scalar> LookAtDirAssumeOrtho(CONST_REF(Vector3<Scalar>) position, CONST_REF(Vector3<Scalar>) front, CONST_REF(Vector3<Scalar>) down, CONST_REF(Vector3<Scalar>) right)
{
	const Matrix3<Scalar> R = MakeFromRows(right, down, front);
	const Vector3<Scalar> T = -R * position;
	return MakeAffineTransform(R, T);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr AffineXForm3D<Scalar> LookAtDirAssumeOrtho(CONST_REF(Vector3<Scalar>) position, CONST_REF(Vector3<Scalar>) front, CONST_REF(Vector3<Scalar>) down)
{
	const Vector3<Scalar> right = Cross(front, down);
	return LookAtDirAssumeOrtho(position, front, down, right);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr AffineXForm3D<Scalar> LookAtDir(CONST_REF(Vector3<Scalar>) position, CONST_REF(Vector3<Scalar>) front, CONST_REF(Vector3<Scalar>) down)
{
	const Vector3<Scalar> s_ = Cross(down, front);
	const Scalar sinus = Length(s_);
	const Vector3<Scalar> s = s_ / sinus;
	const Vector3<Scalar> d = (down - front * Dot(front, down)) / sinus;
	return LookAtDirAssumeOrtho(position, front, d, s);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr AffineXForm3D<Scalar> LookAt(CONST_REF(Vector3<Scalar>) position, CONST_REF(Vector3<Scalar>) focus, CONST_REF(Vector3<Scalar>) down)
{
	return LookAtDir(position, Normalize(focus - position), down);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr AffineXForm3D<Scalar> InverseLookAtDirAssumeOrtho(CONST_REF(Vector3<Scalar>) position, CONST_REF(Vector3<Scalar>) front, CONST_REF(Vector3<Scalar>) down, CONST_REF(Vector3<Scalar>) right)
{
	const Matrix3<Scalar> R = MakeFromCols(right, down, front);
	const Vector3<Scalar> T = position;
	return MakeAffineTransform(R, T);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr AffineXForm3D<Scalar> InverseLookAtDirAssumeOrtho(CONST_REF(Vector3<Scalar>) position, CONST_REF(Vector3<Scalar>) front, CONST_REF(Vector3<Scalar>) down)
{
	const Vector3<Scalar> right = Cross(front, down);
	return InverseLookAtDirAssumeOrtho(position, front, down, right);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr AffineXForm3D<Scalar> InverseLookAtDir(CONST_REF(Vector3<Scalar>) position, CONST_REF(Vector3<Scalar>) front, CONST_REF(Vector3<Scalar>) down)
{
	const Vector3<Scalar> s_ = (Cross(front, down));
	const Scalar sinus = Length(s_);
	const Vector3<Scalar> s = s_ / sinus;
	const Vector3<Scalar> d = (down - front * Dot(front, down)) / sinus;
	return InverseLookAtDirAssumeOrtho(position, front, d, s);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr AffineXForm3D<Scalar> InverseLookAt(CONST_REF(Vector3<Scalar>) position, CONST_REF(Vector3<Scalar>) focus, CONST_REF(Vector3<Scalar>) down)
{
	return InverseLookAtDir(position, Normalize(focus - position), down);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix<Scalar, 4, 4> PerspectiveProjExplicit(CONST_REF(Vector4<Scalar>) coefs)
{
	Matrix4<Scalar> res = MakeUniformMatrix<4, 4>(Scalar(0));
	SetCoeficient(res, 0, 0, coefs[0]);
	SetCoeficient(res, 1, 1, coefs[1]);
	SetCoeficient(res, 2, 2, coefs[2]);
	SetCoeficient(res, 3, 2, Scalar(1));
	SetCoeficient(res, 2, 3, coefs[3]);

	return res;
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Vector<Scalar, 4> ExtractPerspectiveProjCoefs(CONST_REF(Matrix4<Scalar>) m)
{
	return Vector4<Scalar>(
		GetCoeficient(m, 0, 0),
		GetCoeficient(m, 1, 1),
		GetCoeficient(m, 2, 2),
		GetCoeficient(m, 2, 3)
	);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Vector4<Scalar> GetPerspectiveProjCoefsFromInvTanInvAspect(Scalar inv_tan_half_fov, Scalar inv_aspect, Vector2<Scalar> z_range)
{
	Vector4<Scalar> res;
	res[0] = inv_tan_half_fov * inv_aspect;
	res[1] = inv_tan_half_fov;
	res[2] = z_range[1] / (z_range[1] - z_range[0]);
	res[3] = -z_range[0] * res[2];
	return res;
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix4<Scalar> PerspectiveProjFromInvTanInvAspect(Scalar inv_tan_half_fov, Scalar inv_aspect, Vector2<Scalar> z_range)
{
	return PerspectiveProjExplicit(GetPerspectiveProjCoefsFromInvTanInvAspect(inv_tan_half_fov, inv_aspect, z_range));
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix4<Scalar> PerspectiveProjFromTan(Scalar tan_half_fov, Scalar aspect, Vector2<Scalar> z_range)
{
	return PerspectiveProjFromInvTanInvAspect(rcp(tan_half_fov), rcp(aspect), z_range);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix4<Scalar> PerspectiveProjFromFOV(Scalar fov, Scalar aspect, Vector2<Scalar> z_range)
{
	return PerspectiveProjFromTan(TanHalfFOV(fov), aspect, z_range);
}

// Works in both directions
template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Vector4<Scalar> InversePerspectiveProjCoefs(Vector4<Scalar> coefs)
{
	Vector4<Scalar> res;
	res[0] = rcp(coefs[0]);
	res[1] = rcp(coefs[1]);
	res[3] = rcp(coefs[3]);
	res[2] = - coefs[2] * res[3];
	return res;
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix4<Scalar> InversePerspectiveProjExplicit(Vector4<Scalar> coefs)
{
	Matrix4<Scalar> res = MakeUniformMatrix<4, 4>(Scalar(0));
	SetCoeficient(res, 0, 0, coefs[0]);
	SetCoeficient(res, 1, 1, coefs[1]);
	SetCoeficient(res, 2, 3, Scalar(1));
	SetCoeficient(res, 3, 2, coefs[3]);
	SetCoeficient(res, 3, 3, coefs[2]);
	return res;
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Vector4<Scalar> GetInversePerspectiveProjCoefsFromTan(Scalar tan_half_fov, Scalar aspect, Vector2<Scalar> z_range)
{
	Vector4<Scalar> res;
	res[0] = tan_half_fov * aspect;
	res[1] = tan_half_fov;
	const Scalar z = z_range[1] / (z_range[1] - z_range[0]);
	const Scalar w = -z_range[0] * z;
	res[3] = rcp(w);
	res[2] = rcp(z_range[0]);
	return res;
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix4<Scalar> InversePerspectiveProjFromTan(Scalar tan_half_fov, Scalar aspect, Vector2<Scalar> z_range)
{
	return InversePerspectiveProjExplicit(GetInversePerspectiveProjCoefsFromTan(tan_half_fov, aspect, z_range));
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix4<Scalar> InversePerspectiveProjFromFOV(Scalar fov, Scalar aspect, Vector2<Scalar> z_range)
{
	return InversePerspectiveProjFromTan(TanHalfFOV(fov), aspect, z_range);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Vector3<Scalar> InverseInfinitePerspectiveProjCoefs(Vector3<Scalar> coefs)
{
	Vector3<Scalar> res = rcp(coefs);
	return res;
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix4<Scalar> InfinitePerspectiveProjExplicit(Vector3<Scalar> coefs)
{
	Matrix4<Scalar> res = MakeUniformMatrix<4, 4>(Scalar(0));;
	SetCoeficient(res, 0, 0, coefs[0]);
	SetCoeficient(res, 1, 1, coefs[1]);
	SetCoeficient(res, 2, 2, Scalar(1));
	SetCoeficient(res, 2, 3, coefs[2]);
	SetCoeficient(res, 3, 2, Scalar(1));
	return res;
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix4<Scalar> InverseInfinitePerspectiveProjExplicit(Vector3<Scalar> coefs)
{
	Matrix4<Scalar> res = MakeUniformMatrix<4, 4>(Scalar(0));;
	SetCoeficient(res, 0, 0, coefs[0]);
	SetCoeficient(res, 1, 1, coefs[1]);
	SetCoeficient(res, 2, 3, Scalar(1));
	SetCoeficient(res, 3, 2, coefs[2]);
	SetCoeficient(res, 3, 3, -coefs[2]);
	return res;
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Vector3<Scalar> GetInfinitePerspectiveProjCoefsFromInvTanInvAspect(Scalar inv_tan_half_fov, Scalar inv_aspect, Scalar z_near)
{
	Vector3<Scalar> coefs;
	coefs[0] = inv_tan_half_fov * inv_aspect;
	coefs[1] = inv_tan_half_fov;
	coefs[2] = - Scalar(2) * z_near;
	return coefs;
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Vector3<Scalar> GetInfinitePerspectiveProjCoefsFromTan(Scalar tan_half_fov, Scalar aspect, Scalar z_near)
{
	return GetInfinitePerspectiveProjCoefsFromInvTanInvAspect(rcp(tan_half_fov), rcp(aspect), z_near);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix4<Scalar> InfinitePerspectiveProjFromInvTanInvAspect(Scalar inv_tan_half_fov, Scalar inv_aspect, Scalar z_near)
{
	return InfinitePerspectiveProjExplicit(GetInfinitePerspectiveProjCoefsFromInvTanInvAspect(inv_tan_half_fov, inv_aspect, z_near));
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix4<Scalar> InfinitePerspectiveProjFromTan(Scalar tan_half_fov, Scalar aspect, Scalar z_near)
{
	return InfinitePerspectiveProjExplicit(GetInfinitePerspectiveProjCoefsFromTan(tan_half_fov, aspect, z_near));
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix4<Scalar> InfinitePerspectiveProjFromFOV(Scalar fov, Scalar aspect, Scalar z_near)
{
	const Scalar tan_half_fov = TanHalfFOV(fov);
	return InfinitePerspectiveProjFromTan(tan_half_fov, aspect, z_near);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Vector3<Scalar> GetInverseInfinitePerspectiveProjCoefsFromTanInvZnear(Scalar tan_half_fov, Scalar aspect, Scalar inv_z_near)
{
	Vector3<Scalar> coefs;
	coefs[0] = tan_half_fov * aspect;
	coefs[1] = tan_half_fov;
	coefs[2] = - inv_z_near / Scalar(2);
	return coefs;
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix4<Scalar> InverseInfinitePerspectiveProjFromTanInvZnear(Scalar tan_half_fov, Scalar aspect, Scalar inv_z_near)
{
	return InverseInfinitePerspectiveProjExplicit(GetInverseInfinitePerspectiveProjCoefsFromTanInvZnear(tan_half_fov, aspect, inv_z_near));
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix4<Scalar> InverseInfinitePerspectiveProjFromTan(Scalar tan_half_fov, Scalar aspect, Scalar z_near)
{
	return InverseInfinitePerspectiveProjExplicit(GetInverseInfinitePerspectiveProjCoefsFromTanInvZnear(tan_half_fov, aspect, rcp(z_near)));
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix4<Scalar> InverseInfinitePerspectiveProjFromFOV(Scalar fov, Scalar aspect, Scalar z_near)
{
	return InverseInfinitePerspectiveProjFromTan(TanHalfFOV(fov), aspect, z_near);
}


// lbn: left, bottom, near
// rtf: right, top, far
template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix4<Scalar> OrthoProj(Vector3<Scalar> lbn, Vector3<Scalar> rtf)
{
	const Vector3<Scalar> d = rtf - lbn;
	const Vector3<Scalar> s = rtf + lbn;
	const Vector3<Scalar> factors = Vector3<Scalar>(Scalar(2), Scalar(2), Scalar(1));
	const Vector3<Scalar> diag = factors / d;
	const Matrix3<Scalar> Q = DiagonalMatrixV(diag);
	const Vector3<Scalar> T = Vector3<Scalar>(-s[0] / d[0], -s[1] / d[1], -lbn[2] / d[2]);
	const Matrix4<Scalar> res = ResizeMatrix<4, 4>(MakeAffineTransform(Q, T));
	return res;
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix4<Scalar> InverseOrthoProj(Vector3<Scalar> lbn, Vector3<Scalar> rtf)
{
	const Vector3<Scalar> d = rtf - lbn;
	const Vector3<Scalar> s = rtf + lbn;
	const Vector3<Scalar> factors = Vector3<Scalar>(Scalar(2), Scalar(2), Scalar(1));
	const Vector3<Scalar> diag = d / factors;
	const Matrix3<Scalar> Q = DiagonalMatrixV(diag);
	const Vector3<Scalar> T = Vector3<Scalar>(s[0], s[1], lbn[2]) / factors;
	const Matrix4<Scalar> res = ResizeMatrix<4, 4>(MakeAffineTransform(Q, T));
	return res;
}

