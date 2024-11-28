#pragma once

#define GLM_FORCE_SIZE_T_LENGTH 1
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#define GLM_FORCE_LEFT_HANDED 1

#include <that/core/BasicTypes.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace vkl
{
	template <uint N, class T>
	using Vector = glm::vec<N, T>;

	template <class T>
	using Vector2 = Vector<2, T>;

	template <class T>
	using Vector3 = Vector<3, T>;

	template <class T>
	using Vector4 = Vector<4, T>;

	using Vector2f = Vector<2, float>;
	using Vector2d = Vector<2, double>;

	using Vector3f = Vector<3, float>;
	using Vector3d = Vector<3, double>;

	using Vector4f = Vector<4, float>;
	using Vector4d = Vector<4, double>;

	// Follow the glsl/glm standard: 
	// Cols x Rows
	template <uint Cols, uint Rows, class T>
	using Matrix = glm::mat<Cols, Rows, T>;

	template <class T>
	using Matrix2x2 = Matrix<2, 2, T>;

	template <class T>
	using Matrix3x3 = Matrix<3, 3, T>;

	template <class T>
	using Matrix4x4 = Matrix<4, 4, T>;

	using Matrix2x2f = Matrix2x2<float>;
	using Matrix3x3f = Matrix3x3<float>;
	using Matrix4x4f = Matrix4x4<float>;

	using Matrix2x2d = Matrix2x2<double>;
	using Matrix3x3d = Matrix3x3<double>;
	using Matrix4x4d = Matrix4x4<double>;

	template <uint N, class T>
	using MatrixN = Matrix<N, N, T>;

	template <class T>
	using Matrix2 = MatrixN<2, T>;

	template <class T>
	using Matrix3 = MatrixN<3, T>;

	template <class T>
	using Matrix4 = MatrixN<4, T>;

	template <class T>
	using Matrix4x3 = Matrix<4, 3, T>;

	template <class T>
	using Matrix3x4 = Matrix<3, 4, T>;

	using Matrix2f = Matrix2<float>;
	using Matrix3f = Matrix3<float>;
	using Matrix4f = Matrix4<float>;
	using Matrix4x3f = Matrix4x3<float>;
	using Matrix3x4f = Matrix3x4<float>;

	using Matrix2d = Matrix2<double>;
	using Matrix3d = Matrix3<double>;
	using Matrix4d = Matrix4<double>;

	template <uint Cols, uint Rows, class T>
	using MatrixRowMajor = Matrix<Rows, Cols, T>;

	template <class T>
	using Matrix4x3RowMajor = MatrixRowMajor<4, 3, T>;

	using Matrix4x3fRowMajor = Matrix4x3RowMajor<float>;

	template <uint Cols, uint Rows, class T>
	static inline MatrixRowMajor<Cols, Rows, T> ToRowMajor(Matrix<Cols, Rows, T> const& m)
	{
		return glm::transpose(m);
	}

	template <uint Cols, uint Rows, class T>
	static inline Matrix<Cols, Rows, T> ToColMajor(MatrixRowMajor<Cols, Rows, T> const& m)
	{
		return glm::transpose(m);
	}
}

#define USING_MATHS_TYPES(FLOAT) \
using Vector2 = Vector2<FLOAT>;\
using Vector3 = Vector3<FLOAT>;\
using Vector4 = Vector4<FLOAT>;\
\
using Matrix2 = Matrix2<FLOAT>;\
using Matrix3 = Matrix3<FLOAT>;\
using Matrix4 = Matrix4<FLOAT>;\
