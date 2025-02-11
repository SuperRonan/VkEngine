#pragma once

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix2<Scalar> Rotation2(Scalar theta)
{
	CPP_ONLY(using namespace std);
	const Scalar c = cos(theta);
	const Scalar s = sin(theta);
	Matrix2<Scalar> res;
	SetCoeficient(res, 0, 0, c);
	SetCoeficient(res, 1, 1, c);
	SetCoeficient(res, 0, 1, -s);
	SetCoeficient(res, 1, 0, s);
	return res;
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix3<Scalar> Rotation3_1(CONST_REF(Vector3<Scalar>) axis, Scalar angle)
{
	const Matrix3<Scalar> B = BasisFromDir(axis);
	const Matrix2<Scalar> R2 = Rotation2(angle);
	const Matrix3<Scalar> res = B * Matrix3<Scalar>(R2) * Transpose(B);
	return res;
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix3<Scalar> Rotation3_2(CONST_REF(Vector3<Scalar>) axis, Scalar angle)
{
	CPP_ONLY(using namespace std);
	const Scalar c = cos(angle);
	const Scalar s = sin(angle);
	const Matrix3<Scalar> res = DiagonalMatrix<3>(c) + s * CrossProductMatrix(axis) + (Scalar(1) - c) * OuterProduct(axis, axis);
	return res;
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix3<Scalar> Rotation3(CONST_REF(Vector3<Scalar>) axis, Scalar angle)
{
	return Rotation3_2(axis, angle);
}

template <uint Axis, CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
	REQUIRE_CLAUSE(Axis >= 0 && Axis < 3)
constexpr Matrix3<Scalar> Rotation3(Scalar angle)
{
	Vector3<Scalar> axis(0, 0, 0);
	axis[Axis] = Scalar(1);
	return Rotation3(axis, angle);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix3<Scalar> Rotation3X(Scalar angle)
{
	return Rotation3<0>(angle);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix3<Scalar> Rotation3Y(Scalar angle)
{
	return Rotation3<1>(angle);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix3<Scalar> Rotation3Z(Scalar angle)
{
	return Rotation3<2>(angle);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix3<Scalar> Rotation3XYZ(CONST_REF(Vector3<Scalar>) xyz)
{
	return Rotation3X(xyz[0]) * Rotation3Y(xyz[1]) * Rotation3Z(xyz[2]);
}


template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix3<Scalar> Rotation3Intrinsic(CONST_REF(Vector3<Scalar>) angles)
{
	return Rotation3Z(angles[0]) * Rotation3Y(angles[1]) * Rotation3X(angles[2]);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix3<Scalar> Rotation3Extrinsic(CONST_REF(Vector3<Scalar>) angles)
{
	return Rotation3Z(angles[2]) * Rotation3Y(angles[1]) * Rotation3X(angles[0]);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Matrix3<Scalar> Rotation3D(CONST_REF(Vector3<Scalar>) angles, bool extrinsic = true)
{
	return extrinsic ? Rotation3Extrinsic(angles) : Rotation3Intrinsic(angles);
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Vector3<Scalar> Rotate(CONST_REF(Vector3<Scalar>) vec, CONST_REF(Vector3<Scalar>) axis, Scalar angle)
{
	return Rotation3(axis, angle) * vec;
}