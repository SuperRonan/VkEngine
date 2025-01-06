#pragma once

#include <vkl/Core/VulkanCommons.hpp>

#include <cmath>
#include "Types.hpp"

#include <ShaderLib/interop_glsl_cpp>

namespace vkl
{

	template <class T>
	constexpr auto sqr(T const& t)
	{
		return t * t;
	}

	template <class T>
	constexpr auto rcp(T const& t)
	{
		return T(1) / t;
	}

	template <class T>
	constexpr auto TanHalfFOVFast(T const& fov)
	{
		// See Graphics Gems VIII.5
		const T x = fov;
		const T x3 = x * sqr(x);
		const T x5 = x3 * sqr(x);
		const T res = (x + rcp<T>(12) * x3 + rcp<T>(120) * x5) * rcp<T>(2);
		return res;
	}

	template <class T>
	constexpr auto TanHalfFOVCorrect(T const& fov)
	{
		const auto res1 = tan(fov * rcp<T>(2));
		return res1;
	}

	template <class T>
	constexpr auto TanHalfFOV(T const& fov)
	{
		const auto c = TanHalfFOVCorrect(fov);
		const auto f = TanHalfFOVFast(fov);
		return c;
	}

	template <class Scalar>
	constexpr Matrix2<Scalar> RotationMatrix(Scalar angle)
	{
		Matrix2<Scalar> res;
		res.operator[](0);
		res(0, 0) = res(1, 1) = std::cos(angle);
		res(1, 0) = std::sin(angle);
		res(0, 1) = -res(0, 1);
		return res;
	}

	template <class Scalar>
	constexpr Matrix2<Scalar> BasisFromDir(Vector2<Scalar> const& dir)
	{
		const Vector2<Scalar> v(-dir[0], dir[1]);
		return MakeFromColumns<Scalar, 2>({dir, v});
	}

	// Assume dot(Z, X) = 0
	// |X| = |Z| = 1
	template<class Scalar>
	constexpr Matrix3<Scalar> BasisFrom2DBasisZX(Vector3<Scalar> const& Z, Vector3<Scalar> const& X)
	{
		const Vector3<Scalar> Y = Z.cross(X);
		Matrix3<Scalar> res = MakeFromColumns<Scalar, 3>({X, Y, Z});
		return res;
	}

	template<class Scalar>
	constexpr Matrix3<Scalar> BasisFromDirFast(Vector3<Scalar> const& dir)
	{
		const Vector3<Scalar> Z = dir;
		Vector3<Scalar> o;
		o = Vector3<Scalar>(1, 0, 0);
		const Scalar l(0.5);
		if (std::abs(Dot(o, Z)) >= l)
		{
			o = Vector3<Scalar>(0, 1, 0);
			//if(dot(o, Z) >= l)
			//{
			//	o = Vector3<Scalar>(0, 0, 1);
			//}
		}
		const Vector3<Scalar> X = Normalize(Cross(o, Z));
		return BasisFrom2DBasisZX(Z, X);
	}

	// https://backend.orbit.dtu.dk/ws/portalfiles/portal/126824972/onb_frisvad_jgt2012_v2.pdf
	template<class Scalar>
	constexpr Matrix3<Scalar> BasisFromDir_frisvad(Vector3<Scalar> const& Z)
	{
		Vector3<Scalar> X;
		if (Z.z() < Scalar(-0.9999))
		{
			X = Vector3<Scalar>(0, -1, 0);
		}
		else
		{
			const Scalar a = rcp(Scalar(1) + Z.z());
			const Scalar b = -Z.x() * Z.y() * a;
			X = Vector3f(Scalar(1) - sqr(Z.x()) * a, b, -Z.x());
		}
		return BasisFrom2DBasisZX(Z, X);
	}

	template<class Scalar>
	constexpr Matrix3<Scalar> BasisFromDir_hughes_moeller(Vector3<Scalar> const& Z)
	{
		Vector3<Scalar> Y;
		if(std::abs(Z.x()) > std::abs(Z.z()))	Y = Vector3<Scalar>(-Z.y(), Z.x(), 0);
		else									Y = Vector3<Scalar>(0, -Z.z(), Z.y());
		Y = Normalize(Y);
		Vector3<Scalar> X = Cross(Y, Z);
		return Matrix3<Scalar>(X, Y, Z);
	}

	template <class Scalar>
	constexpr Matrix3<Scalar> RotationMatrix(Vector3<Scalar> const& axis, Scalar angle)
	{
		// Basis not tested for an arbitrary axis
		Matrix3<Scalar> basis = BasisFromDirFast(axis);
		Matrix2<Scalar> R2 = RotationMatrix(angle);
		Matrix3<Scalar> res = (basis) * Matrix3<Scalar>(R2) * Transpose(basis);
		return res;
	}

	template <class Scalar>
	constexpr Matrix3<Scalar> RotationMatrixX(Scalar angle)
	{
		return RotationMatrix(Vector3<Scalar>(1, 0, 0), angle);
	}

	template <class Scalar>
	constexpr Matrix3<Scalar> RotationMatrixY(Scalar angle)
	{
		return RotationMatrix(Vector3<Scalar>(0, 1, 0), angle);
	}

	template <class Scalar>
	constexpr Matrix3<Scalar> RotationMatrixZ(Scalar angle)
	{
		return RotationMatrix(Vector3<Scalar>(0, 0, 1), angle);
	}

	template <class Scalar>
	constexpr Matrix3<Scalar> RotationMatrixIntrinsic(Vector3<Scalar> const& angles)
	{
		return RotationMatrixZ(angles[0]) * RotationMatrixY(angles[1]) * RotationMatrixX(angles[2]);
	}

	template <class Scalar>
	constexpr Matrix3<Scalar> RotationMatrixExtrinsic(Vector3<Scalar> const& angles)
	{
		return RotationMatrixIntrinsic(Vector3<Scalar>(angles[2], angles[1], angles[0]));
	}

	template <class Scalar>
	constexpr Matrix3<Scalar> RotationMatrix(Vector3<Scalar> const& angles, bool extrinsic = true)
	{
		return extrinsic ? RotationMatrixExtrinsic(angles) : RotationMatrixIntrinsic(angles);
	}

	// Returns a NxN matrix
	template <uint N, class Scalar = float>
	constexpr Matrix<Scalar, N> ScalingMatrix(Vector<Scalar, N> const& vector)
	{
		return DiagonalMatrix(vector);
	}

	// Returns a NxN matrix
	template <uint N, class Scalar>
	constexpr Matrix<Scalar, N> ScalingMatrix(Scalar s)
	{
		return MakeMatrix<Scalar, N>(s);
	}

	// Returns a R x R+1 matrix
	template <uint R, class Scalar = float>
	constexpr Matrix<Scalar, R, R+1> TranslationMatrix(ColVector<Scalar, R> const& vector)
	{
		Matrix<Scalar, R, R + 1> res = MakeMatrix<Scalar, R, R + 1>();
		for (uint i = 0; i < R; ++i)
		{
			res(i, R) = vector[i];
		}
		return res;
	}

	template <uint R, class Scalar = float>
	constexpr Matrix<Scalar, R, R + 1> InverseTranslationMatrix(ColVector<Scalar, R> const& vector) 
	{
		return TranslationMatrix(-vector);
	}

	template <class Scalar, uint R, uint C, int Options>
		requires (C == R || C == (R + 1))
	Matrix<Scalar, R, C, Options> InverseTranslateMatrix(Matrix<Scalar, R, C, Options> const& t_mat)
	{
		Matrix<Scalar, R, C, Options> res = MakeMatrix<Scalar, R, C, Options>();
		constexpr const uint t_dim = C - 1;
		for (uint i = 0; i < t_dim; ++i)
		{
			res(i, t_dim) = -t_mat(i, t_dim);
		}
		return res;
	}

	template <uint N, class Scalar>
	Vector<Scalar, N + 1> Homogenize(Vector<Scalar, N> const& vec, Scalar const& h=Scalar(1.0))
	{
		Vector<Scalar, N + 1> res;
		res << vec, h;
		return vec;
	}

	template <uint N, class Scalar>
		requires (N > 1)
	Vector<Scalar, N - 1> DeHomogenize(Vector<Scalar, N> const& h_vec)
	{
		Vector<Scalar, N - 1> res = h_vec.block(0, 0, N - 1, 1);
		if (h_vec[N - 1] != Scalar(0))
		{
			res /= h_vec[N - 1];
		}
		return res;
	}

	template <class Scalar>
	Matrix3<Scalar> DirectionMatrix(Matrix3<Scalar> const& mat)
	{
		return Transpose(Inverse(mat));
	}
	
	template <class Scalar>
	Matrix3<Scalar> DirectionMatrix(Matrix4<Scalar> const& mat)
	{
		return DirectionMatrix(ResizeMatrix<3>(mat));
	}

	template <class Scalar>
	Matrix3<Scalar> DirectionMatrix(Matrix4x3<Scalar> const& mat)
	{
		return DirectionMatrix(ResizeMatrix<3>(mat));
	}

	template <std::convertible_to<float> Scalar, uint R, uint C, int Options>
		requires (R == 3 || R == 4) && (C == 3 || C == 4)
	static VkTransformMatrixKHR ConvertXFormToVk(Matrix<Scalar, R, C, Options> const& mat)
	{
		VkTransformMatrixKHR res;
		for (uint32_t i = 0; i < 3; ++i)
		{
			for (uint32_t j = 0; j < 4; ++j)
			{
				res.matrix[i][j] = static_cast<float>(mat(i, j));
			}
		}
		return res;
	}

	template <class Scalar>
	constexpr Matrix4x3<Scalar> MakeRigidTransform(Matrix4x3<Scalar> const& rs, Vector3<Scalar> const& t)
	{
		Matrix4x3<Scalar> res = Matrix4x3<Scalar>(rs);
		res[3] = t;
		return res;
	}

#ifndef DEFAULT_SCALAR
#define DEFAULT_SCALAR Scalar
#endif

#define EQ_DFS = DEFAULT_SCALAR
#define EQ_DD = 3
#define AFFINE_XFORM_INL_TEMPLATE_DECL template <class Scalar_t EQ_DFS, uint Dimensions EQ_DD>
#define AFFINE_XFORM_INL_Dims Dimensions
#define AFFINE_XFORM_INL_Scalar Scalar_t
#define AFFINE_XFORM_INL_QBlock MatrixN<Dimensions, Scalar_t>
#define AFFINE_XFORM_INL_XFormMatrix Matrix<Dimensions + 1, Dimensions, Scalar_t>
#define AFFINE_XFORM_INL_FullMatrix Matrix<Dimensions + 1, Dimensions + 1, Scalar_t>
#define AFFINE_XFORM_INL_Vector Vector<Dimensions, Scalar_t>

#include <ShaderLib/Maths/TemplateImpl/AffineXForm.inl>

#undef EQ_DFS 
#undef EQ_DD 
#undef AFFINE_XFORM_INL_TEMPLATE_DECL 
#undef AFFINE_XFORM_INL_Dims  
#undef AFFINE_XFORM_INL_Scalar 
#undef AFFINE_XFORM_INL_QBlock 
#undef AFFINE_XFORM_INL_XFormMatrix 
#undef AFFINE_XFORM_INL_FullMatrix 
#undef AFFINE_XFORM_INL_Vector 


#define EQ_DFS = DEFAULT_SCALAR
#define CLIP_SPACE_INL_TEMPLATE_DECL template <class Scalar_t EQ_DFS>
#define CLIP_SPACE_INL_Scalar Scalar_t
#define CLIP_SPACE_INL_Vector(N) Vector<N, Scalar_t>
#define CLIP_SPACE_INL_Matrix(N) MatrixN<N, Scalar_t>

#include <ShaderLib/Maths/TemplateImpl/ClipSpaceMatrices.inl>

#undef EQ_DFS 
#undef CLIP_SPACE_INL_TEMPLATE_DECL 
#undef CLIP_SPACE_INL_Scalar 
#undef CLIP_SPACE_INL_Vector
#undef CLIP_SPACE_INL_Matrix

	

	template <class Scalar>
	constexpr Matrix4x3<Scalar> LookAt(Vector3<Scalar> const& position, Vector3<Scalar> const& center, Vector3<Scalar> const& up)
	{
		return glm::lookAt(position, center, up);
	}

	template <class Scalar>
	constexpr Matrix4x3<Scalar> LookAtDir(Vector3<Scalar> const& position, Vector3<Scalar> const& center_direction, Vector3<Scalar> const& up)
	{
		return LookAt(position, position + center_direction, up);
	}

	template <class Scalar>
	constexpr Matrix4x3<Scalar> InverseLookAt(Vector3<Scalar> const& position, Vector3<Scalar> const& center, Vector3<Scalar> const& up)
	{
		Matrix4x3<Scalar> l = LookAt(position, center, up);
		return InverseRigidTransformFast(l);
	}

	template <class Scalar>
	constexpr Matrix4x3<Scalar> InverseLookAtDir(Vector3<Scalar> const& position, Vector3<Scalar> const& center_direction, Vector3<Scalar> const& up)
	{
		return InverseLookAt(position, position + center_direction, up);
	}
}
