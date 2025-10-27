#pragma once
#include <ShaderLib/interop_slang_cpp>

__generic <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Vector2<Scalar> Rotate90(CONST_REF(Vector2<Scalar>) dir)
{
	Vector2<Scalar> res;
	res[0] = -dir[1];
	res[1] = dir[0];
	return res;
}

__generic <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix2<Scalar> BasisFromDir(CONST_REF(Vector2<Scalar>) dir)
{
	Matrix2<Scalar> res;
	SetColumn(res, 0, dir);
	SetColumn(res, 1, Rotate90(dir));
	return res;
}


// Assume dot(Z, X) = 0
// |X| = |Z| = 1
__generic <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix3<Scalar> BasisFrom2DBasisZX(CONST_REF(Vector3<Scalar>) Z, CONST_REF(Vector3<Scalar>) X)
{
	const Vector3<Scalar> Y = Cross(Z, X);
	Matrix3<Scalar> res = MakeFromCols(X, Y, Z);
	return res;
}


__generic <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix3<Scalar> BasisFromDir_fast(CONST_REF(Vector3<Scalar>) dir)
{
	
	const Vector3<Scalar> Z = dir;
	Vector3<Scalar> o;
	o = Vector3<Scalar>(Scalar(1), Scalar(0), Scalar(0));
	const Scalar l = Scalar(0.5);
	if(abs(dot(o, Z)) >= l)
	{
		o = Vector3<Scalar>(Scalar(0), Scalar(1), Scalar(0));
		// if(dot(o, Z) >= l)
		// {
		// 	o = vec3(0, 0, 1);
		// }
	}
	const Vector3<Scalar> X = Normalize(Cross(o, Z));
	return BasisFrom2DBasisZX(Z, X);
}

// https://backend.orbit.dtu.dk/ws/portalfiles/portal/126824972/onb_frisvad_jgt2012_v2.pdf
__generic <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix3<Scalar> BasisFromDir_frisvad(CONST_REF(Vector3<Scalar>) Z)
{
	Vector3<Scalar> X;
	if (Z[2] < Scalar(-0.9999))
	{
		X = Vector3<Scalar>(Scalar(0), Scalar(-1), Scalar(0));
	}
	else
	{
		const Scalar a = rcp(Scalar(1) + Z[2]);
		const Scalar b = -Z[0] * Z[1] * a;
		X = Vector3<Scalar>(Scalar(1) - sqr(Z[0]) * a, b, -Z[0]);
	}
	return BasisFrom2DBasisZX(Z, X);
}

__generic<CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix3<Scalar> BasisFromDir_hughes_moeller(CONST_REF(Vector3<Scalar>) Z)
{
	CPP_ONLY(using namespace std);
	Vector3<Scalar> Y;
	if (abs(Z[0]) > abs(Z[2]))	Y = Vector3<Scalar>(-Z[1], Z[0], Scalar(0));
	else						Y = Vector3<Scalar>(Scalar(0), -Z[2], Z[1]);
	Y = Normalize(Y);
	Vector3<Scalar> X = Cross(Y, Z);
	return Matrix3<Scalar>(X, Y, Z);
}

__generic<CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix3<Scalar> BasisFromDir(CONST_REF(Vector3<Scalar>) Z, int method = 0)
{
	if(method == 0)
	{
		return BasisFromDir_fast(Z);
	}
	else if(method == 1)
	{
		return BasisFromDir_frisvad(Z);
	}
	else
	{
		return BasisFromDir_hughes_moeller(Z);
	}
}

__generic <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix3<Scalar> CrossProductMatrix(CONST_REF(Vector3<Scalar>) u)
{
	Matrix3<Scalar> res = MakeUniformMatrix<3, 3>(Scalar(0));
	SetCoeficient(res, 1, 0, +u[2]);
	SetCoeficient(res, 2, 0, -u[1]);
	SetCoeficient(res, 0, 1, -u[2]);
	SetCoeficient(res, 2, 1, +u[0]);
	SetCoeficient(res, 0, 2, +u[1]);
	SetCoeficient(res, 1, 2, -u[0]);
	return res;
}

