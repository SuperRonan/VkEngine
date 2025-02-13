#pragma once

#include <ShaderLib/interop_slang_cpp>

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Scalar TanHalfFOVFast(CONST_REF(Scalar) fov)
{
	CPP_ONLY(using namespace std);
	// See Graphics Gems VIII.5
	const Scalar x = fov;
	const Scalar x3 = x * sqr(x);
	const Scalar x5 = x3 * sqr(x);
	const Scalar res = (x + rcp<Scalar>(Scalar(12)) * x3 + rcp<Scalar>(Scalar(120)) * x5) * rcp<Scalar>(Scalar(2));
	return res;
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Scalar TanHalfFOVCorrect(CONST_REF(Scalar) fov)
{
	CPP_ONLY(using namespace std);
	const Scalar res1 = tan(fov * rcp<Scalar>(Scalar(2)));
	return res1;
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Scalar TanHalfFOV(CONST_REF(Scalar) fov)
{
	const Scalar c = TanHalfFOVCorrect(fov);
	const Scalar f = TanHalfFOVFast(fov);
	return c;
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Vector3<Scalar> SphericalToCartesian(CONST_REF(Vector2<Scalar>) theta_phi)
{
	CPP_ONLY(using namespace std);
	const Scalar theta = theta_phi[0];
	const Scalar phi = theta_phi[1];
	const Scalar ct = cos(theta);
	const Scalar st = sin(theta);
	const Scalar cp = cos(phi);
	const Scalar sp = sin(phi);
	Vector3<Scalar> res;
	res[0] = st * cp;
	res[1] = ct;
	res[2] = st * sp;
	return res;
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar)>
constexpr Vector3<Scalar> SphericalToCartesian(CONST_REF(Vector3<Scalar>) theta_phi_rho)
{
	return SphericalToCartesian(Vector2<Scalar>(theta_phi_rho[0], theta_phi_rho[1])) * theta_phi_rho[2];
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), int N>
constexpr Vector<Scalar, N> ClipSpaceToUV(CONST_REF(Vector<Scalar, N>) cp)
{
	return cp / Scalar(2) + MakeUniformVector<N>(Scalar(2));
}

template <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Scalar), int N>
constexpr Vector<Scalar, N> UVToClipSpace(CONST_REF(Vector<Scalar, N>) uv)
{
	return uv * Scalar(2) - MakeUniformVector<N>(Scalar(1));
}

