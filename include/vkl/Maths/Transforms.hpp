#pragma once

#include <vkl/Core/VulkanCommons.hpp>

#include <cmath>
#include "Types.hpp"

#include <that/core/Core.hpp>

#include <ShaderLib/interop_slang_cpp>

#include <numbers>

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

	template <std::floating_point T>
	constexpr auto Radians(T degrees)
	{
		return degrees * std::numbers::pi_v<T> / T(180);
	}

	template <class Scalar, int Dimensions>
	using AffineXForm = Matrix<Scalar, Dimensions, Dimensions + 1>;

	template <class Scalar>
	using AffineXForm3D = AffineXForm<Scalar, 3>;

	using AffineXForm3Df = AffineXForm3D<float>;

	template <concepts::CompileTimeSizedVectorCompatible Vec>
	constexpr auto Homogeneous(Vec const& vec)
	{
		return vec.homogeneous().eval();
	}

	template <concepts::CompileTimeSizedVectorCompatible Vec>
	constexpr auto Homogeneous(Vec const& vec, typename Vec::Scalar const& h)
	{
		using Scalar = typename Vec::Scalar;
		constexpr const size_t R = Vec::RowsAtCompileTime;
		constexpr const size_t C = Vec::ColsAtCompileTime;
		using EVRT = typename std::remove_cvref<typename Vec::EvalReturnType>::type;
		using Ret_t = Matrix<Scalar, (C == 1) ? (R + 1) : 1, (C == 1) ? 1 : (C + 1), EVRT::Options>;
		Ret_t res;
		res << vec, h;
		return res;
	}

	template <concepts::CompileTimeSizedVectorCompatible Vec>
		requires (Vec::ColsAtCompileTime > 1 || Vec::RowsAtCompileTime > 1)
	constexpr auto HomogeneousNormalize(Vec const& h_vec)
	{
		return h_vec.hnormalized();
	}

	template <concepts::CompileTimeSizedMatrixCompatible Matrix>
	constexpr auto HomogeneousMatrix(Matrix const& m, typename Matrix::Scalar const& h = typename Matrix::Scalar(1))
	{
		constexpr const size_t R = Matrix::RowsAtCompileTime;
		constexpr const size_t C = Matrix::ColsAtCompileTime;
		using EVRT = typename std::remove_cvref<typename Matrix::EvalReturnType>::type;
		using Ret_t = ::vkl::Matrix<typename Matrix::Scalar, R + 1, C + 1, EVRT::Options>;
		Ret_t res = Ret_t(m, h);
		return res;
	}

	template <std::convertible_to<float> Scalar, int R, int C, int Options>
		requires (R == 3 || R == 4) && (C == 3 || C == 4)
	static constexpr VkTransformMatrixKHR ConvertXFormToVk(Matrix<Scalar, R, C, Options> const& mat)
	{
		VkTransformMatrixKHR res = {};
		for (uint32_t i = 0; i < 3; ++i)
		{
			for (uint32_t j = 0; j < 4; ++j)
			{ 
				if (i < R && j < C)
				{
					res.matrix[i][j] = static_cast<float>(mat(i, j));
				}
				else
				{
					res.matrix[i][j] = 0.0f;
				}
			}
		}
		return res;
	}

	template <concepts::CompileTimeSizedMatrixCompatible MatL, concepts::SameCompatibleMatrix<MatL> MatR>
		requires ((MatL::RowsAtCompileTime + 1) == MatL::ColsAtCompileTime)
	static constexpr auto mul(MatL const& l, MatR const& r)
	{
		return l * r;
	}

#include <ShaderLib/Maths/Shared/Common.inl>
#include <ShaderLib/Maths/Shared/3DMatrices.inl>
#include <ShaderLib/Maths/Shared/Rotations.inl>

	template <concepts::CompileTimeSizedSquareMatrixCompatible Mat>
	static constexpr auto DirectionMatrix(Mat const& m)
	{
		return Inverse(Transpose(m));
	}

	template <concepts::CompileTimeSizedMatrixCompatible Mat>
		requires ((Mat::RowsAtCompileTime + 1) == Mat::ColsAtCompileTime)
	static constexpr auto DirectionMatrix(Mat const& m)
	{
		return DirectionMatrix(ExtractQBlock(m.eval()));
	}
}
