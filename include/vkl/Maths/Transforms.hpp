#pragma once

#include <vkl/Core/VulkanCommons.hpp>

#include <cmath>
#include "Types.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include <ShaderLib/interop_glsl_cpp>

namespace glm
{
	
}

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

	template <uint N, class Scalar>
	constexpr MatrixN<N, Scalar> DiagonalMatrix(Vector<N, Scalar> const& d)
	{
		MatrixN<N, Scalar> res;
		for (uint i = 0; i < N; ++i)
		{
			res[i][i] = d[i];
		}
		return res;
	}

	template <class Float>
	constexpr MatrixN<2, Float> RotationMatrix(Float angle)
	{
		MatrixN<2, Float> res;
		res[0][0] = res[1][1] = cos(angle);
		res[0][1] = sin(angle);
		res[1][0] = -res[0][1];
		return res;
	}

	template <uint N, class Float>
	constexpr MatrixN<N, Float> ScalingMatrix(Vector<N-1, Float> const& vector)
	{
		MatrixN<N, Float> res(Float(1.0));
		for (int i = 0; i < N-1; ++i)
		{
			res[i][i] = vector[i];
		}
		return res;
	}

	template <uint N, class Float>
	constexpr MatrixN<N, Float> ScalingMatrix(Float s)
	{
		MatrixN<N, Float> res(Float(1.0));
		for (int i = 0; i < N - 1; ++i)
		{
			res[i][i] = s;
		}
		return res;
	}

	template <uint N, class Float>
	constexpr MatrixN<N, Float> TranslationMatrix(Vector<N - 1, Float> const& vector)
	{
		MatrixN<N, Float> res(Float(1.0));
		for (int i = 0; i < N - 1; ++i)
		{
			res[N-1][i] = vector[i];
		}
		return res;
	}

	template <uint N, class Float>
	MatrixN<N, Float> InverseTranslateMatrix(Vector<N - 1, Float> const& vector)
	{
		MatrixN<N, Float> res(Float(1.0));
		for (int i = 0; i < N - 1; ++i)
		{
			res[N - 1][i] = -vector[i];
		}
		return res;
	}

	template <uint N, class Float>
	MatrixN<N, Float> InverseTranslateMatrix(MatrixN<N, Float> const& t_mat)
	{
		MatrixN<N, Float> res(Float(1.0));
		for (int i = 0; i < N - 1; ++i)
		{
			res[N - 1][i] = -t_mat[N-1][i];
		}
		return res;
	}

	template <uint N, class Float>
	Vector<N + 1, Float> Homogenize(Vector<N, Float> const& vec, Float h=Float(1.0))
	{
		return Vector<N + 1, Float>(vec, h);
	}

	template <uint N, class Float>
	Vector<N, Float> DeHomogenize(Vector<N + 1, Float> const& h_vec)
	{
		Vector<N, Float> res;
		if (h_vec[N] != 0)
		{
			for (int i = 0; i < N; ++i)
				res[i] = h_vec[i] / h_vec[N];
		}
		else
		{
			for (int i = 0; i < N; ++i)
				res[i] = h_vec[i];
		}
		return res;
	}

	template <class Float>
	Matrix3<Float> DirectionMatrix(Matrix3<Float> const& mat)
	{
		return glm::transpose(glm::inverse(mat));
	}
	
	template <class Float>
	Matrix3<Float> DirectionMatrix(Matrix4<Float> const& mat)
	{
		return DirectionMatrix(Matrix3<Float>(mat));
	}

	template <class Float>
	Matrix3<Float> DirectionMatrix(Matrix4x3<Float> const& mat)
	{
		return DirectionMatrix(Matrix3<Float>(mat));
	}

	inline VkTransformMatrixKHR ConvertXFormToVk(Matrix4x3f const& mat)
	{
		// glm::mat4x3 is stored as 4 vec3, so can't directly memcpy :(
		VkTransformMatrixKHR res;
		for (uint32_t i = 0; i < 3; ++i)
		{
			for (uint32_t j = 0; j < 4; ++j)
			{
				res.matrix[i][j] = mat[j][i];
			}
		}
		return res;
	}

	template <class Float>
	constexpr Matrix4x3<Float> MakeRigidTransform(Matrix4x3<Float> const& rs, Vector3<Float> const& t)
	{
		Matrix4x3<Float> res = Matrix4x3<Float>(rs);
		res[3] = t;
		return res;
	}

#ifndef DEFAULT_SCALAR
#define DEFAULT_SCALAR float
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
