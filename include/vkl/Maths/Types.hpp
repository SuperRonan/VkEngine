#pragma once

#ifndef VKL_DEFAULT_MATRIX_ROW_MAJOR
#define VKL_DEFAULT_MATRIX_ROW_MAJOR false
#endif

#ifndef VKL_DEFAULT_MATRIX_AUTO_ALIGN
#define VKL_DEFAULT_MATRIX_AUTO_ALIGN true
#endif

#include <that/core/BasicTypes.hpp>

namespace vkl
{
	namespace concepts
	{
		template <class C, class Dst, class Src>
		concept Converter = requires(C c, Src src)
		{
			{ c(src) } -> std::convertible_to<Dst>;
		};
	}

	template <class Dst, class Src>
	struct DefaultStaticCastConverter
	{
		constexpr DefaultStaticCastConverter() noexcept = default;
		constexpr Dst operator()(Src const& src) const noexcept
		{
			return static_cast<Dst>(src);
		}
	};
}

#define EIGEN_MATRIX_PLUGIN <vkl/Maths/Impl/Eigen/MatrixPlugin.inl>
#define EIGEN_MATRIXBASE_PLUGIN <vkl/Maths/Impl/Eigen/MatrixBasePlugin.inl>
#define EIGEN_PLAINOBJECTBASE_PLUGIN <vkl/Maths/Impl/Eigen/PlainObjectBasePlugin.inl>

#include <Eigen/Dense>


namespace vkl
{

	namespace eg = ::Eigen;

	static constexpr int MatrixOptions(bool row_major, bool auto_align)
	{
		int res = 0;
		res |= (row_major ? eg::RowMajor : eg::ColMajor);
		res |= (auto_align ? eg::AutoAlign : eg::DontAlign);
		return res;
	}

	static constexpr int MatrixOptions(uint R, uint C, bool row_major, bool auto_align)
	{
		if (R == 1)
		{
			row_major = true;
		}
		else if (C == 1)
		{
			row_major = false;
		}
		return MatrixOptions(row_major, auto_align);
	}

	static constexpr int ExtractRowMajorFromMatrixOption(int options)
	{
		return (options & eg::RowMajor) != 0;
	}

	// Follow the math convention
	// Rows x Cols
	// Direct template alias, can be used for template param deduction
	template <class T, uint R, uint C = R, int Options = MatrixOptions(R, C, VKL_DEFAULT_MATRIX_ROW_MAJOR, VKL_DEFAULT_MATRIX_AUTO_ALIGN)>
	using Matrix = eg::Matrix<T, R, C, Options>;

	template <class T, uint N, int Options = MatrixOptions(false, VKL_DEFAULT_MATRIX_AUTO_ALIGN)>
	using ColVector = Matrix<T, N, 1, Options>;

	template <class T, uint N, int Options = MatrixOptions(true, VKL_DEFAULT_MATRIX_AUTO_ALIGN)>
	using RowVector = Matrix<T, 1, N, Options>;

	template <class T, uint N, int Options = MatrixOptions(false, VKL_DEFAULT_MATRIX_AUTO_ALIGN)>
	using Vector = ColVector<T, N, Options>;

	namespace concepts
	{
	}

	template <class T>
	using Vector2 = Vector<T, 2>;

	template <class T>
	using Vector3 = Vector<T, 3>;

	template <class T>
	using Vector4 = Vector<T, 4>;

	using Vector2f = Vector<float, 2>;
	using Vector2d = Vector<double, 2>;

	using Vector3f = Vector<float, 3>;
	using Vector3d = Vector<double, 3>;

	using Vector4f = Vector<float, 4>;
	using Vector4d = Vector<double, 4>;

	template <class T>
	using Matrix2 = Matrix<T, 2>;
	template <class T>
	using Matrix2x2 = Matrix2<T>;

	template <class T>
	using Matrix3 = Matrix<T, 3>;
	template <class T>
	using Matrix3x3 = Matrix3<T>;

	template <class T>
	using Matrix4 = Matrix<T, 4>;
	template <class T>
	using Matrix4x4 = Matrix4<T>;

	template <class T>
	using Matrix4x3 = Matrix<T, 4, 3>;

	template <class T>
	using Matrix3x4 = Matrix<T, 3, 4>;

	using Matrix2f = Matrix2<float>;
	using Matrix3f = Matrix3<float>;
	using Matrix4f = Matrix4<float>;
	using Matrix4x3f = Matrix4x3<float>;
	using Matrix3x4f = Matrix3x4<float>;

	using Matrix2d = Matrix2<double>;
	using Matrix3d = Matrix3<double>;
	using Matrix4d = Matrix4<double>;

	template <class T, uint R, uint C = R>
	using MatrixRowMajor = Matrix<T, R, C, MatrixOptions(R, C, true, VKL_DEFAULT_MATRIX_AUTO_ALIGN)>;

	template <class T, uint R, uint C = R>
	using MatrixColMajor = Matrix<T, R, C, MatrixOptions(R, C, false, VKL_DEFAULT_MATRIX_AUTO_ALIGN)>;

	template <class T>
	using Matrix3x4RowMajor = MatrixRowMajor<T, 3, 4>;

	using Matrix3x4fRowMajor = Matrix3x4RowMajor<float>;

	template <class T, uint R, uint C, int Options>
	static RowVector<T, C, Options> GetRow(Matrix<T, R, C, Options> const& m, uint i)
	{
		return m.row(i);
	}

	template <class T, uint R, uint C, int Options>
	static ColVector<T, R, Options> GetColumn(Matrix<T, R, C, Options> const& m, uint i)
	{
		return m.col(i);
	}

	template <class T, uint R, uint C, int Options>
	static void SetRow(Matrix<T, R, C, Options>& m, uint i, RowVector<T, C, Options> const& v)
	{
		m.row(i) = v;
	}

	template <class T, uint R, uint C, int Options>
	static void SetColumn(Matrix<T, R, C, Options>& m, uint i, ColVector<T, C, Options> const& v)
	{
		m.col(i) = v;
	}

	template<class T, uint R, uint C, int Options>
	static Matrix<T, R, C, Options> MakeFromRows(std::array<RowVector<T, C, Options>, R> const& rows)
	{
		Matrix<T, R, C, Options> res;
		for (uint i = 0; i < R; ++i)
		{
			SetRow(res, i, rows[i]);
		}
		return res;
	}

	template<class T, uint R, uint C, int Options>
	static Matrix<T, R, C, Options> MakeFromColumns(std::array<ColVector<T, R, Options>, C> const& columns)
	{
		Matrix<T, R, C, Options> res;
		for (uint i = 0; i < C; ++i)
		{
			SetColumn(res, i, columns[i]);
		}
		return res;
	}

	// Make a diagonal matrix 
	// Returns the Identity matrix by default
	// The matrix is square by default, but is can be rectangular
	template <class T, uint R, uint C = R, int Options = MatrixOptions(R, C, VKL_DEFAULT_MATRIX_ROW_MAJOR, VKL_DEFAULT_MATRIX_AUTO_ALIGN)>
	static Matrix<T, R, C, Options> MakeMatrix(T const& t = T(1))
	{
		Matrix<T, R, C, Options> res = Matrix<T, R, C, Options>::Zero();
		for (uint i = 0; i < std::min(R, C); ++i)
		{
			res(i, i) = t;
		}
		return res;
	}

	template <class T, uint R, uint C = R, int Options = MatrixOptions(R, C, VKL_DEFAULT_MATRIX_ROW_MAJOR, VKL_DEFAULT_MATRIX_AUTO_ALIGN)>
	static Matrix<T, R, C, Options> DiagonalMatrix(T const& t = T(1))
	{
		return MakeMatrix<T, R, C, Options>(t);
	}

	// Make a square diagonal matrix with the vector coefs in the diagonal
	template <class T, uint N, int Options = MatrixOptions(N, N, VKL_DEFAULT_MATRIX_ROW_MAJOR, VKL_DEFAULT_MATRIX_AUTO_ALIGN)>
	static Matrix<T, N, N, Options> MakeMatrix(Vector<T, N, Options> const& diag)
	{
		Matrix<T, N, N, Options> res = Matrix<T, N, N, Options>::Zero();
		for (uint i = 0; i < N; ++i)
		{
			res(i, i) = diag[i];
		}
		return res;
	}

	template <class T, uint N, int Options = MatrixOptions(N, N, VKL_DEFAULT_MATRIX_ROW_MAJOR, VKL_DEFAULT_MATRIX_AUTO_ALIGN)>
	static Matrix<T, N, N, Options> DiagonalMatrix(Vector<T, N, Options> const& diag)
	{
		return MakeMatrix(diag);
	}

	template <class T, uint N, int Options = MatrixOptions(N, N, VKL_DEFAULT_MATRIX_ROW_MAJOR, VKL_DEFAULT_MATRIX_AUTO_ALIGN)>
	static Matrix<T, N+1, N+1, Options> DiagonalMatrix(Vector<T, N, Options> const& diag, T const& last)
	{
		Vector<T, N + 1> dd;
		dd << diag , last;
		return DiagonalMatrix(dd);
	}

	

	template <
		class DST_T, 
		uint DST_R, 
		uint DST_C = DST_R, 
		class SRC_T = DST_T, 
		uint SRC_R = DST_R, 
		uint SRC_C = DST_C, 
		int DST_Options = MatrixOptions(DST_R, DST_C, VKL_DEFAULT_MATRIX_ROW_MAJOR, VKL_DEFAULT_MATRIX_AUTO_ALIGN), 
		int SRC_Options = MatrixOptions(SRC_R, SRC_C, VKL_DEFAULT_MATRIX_ROW_MAJOR, VKL_DEFAULT_MATRIX_AUTO_ALIGN), 
		concepts::Converter<DST_T, SRC_T> Converter = DefaultStaticCastConverter<DST_T, SRC_T>
	>
	static Matrix<DST_T, DST_R, DST_C, DST_Options> ConvertAndResizeMatrix(Matrix<SRC_T, SRC_R, SRC_C, SRC_Options> const& src, DST_T const& diag = DST_T(1), Converter const& converter = Converter(), uint row_offset = 0, uint col_offset = 0)
	{
		using Res_t = Matrix<DST_T, DST_R, DST_C, DST_Options>;
		Res_t res;
		Res_t::InitFromOther(res, src, diag, converter, row_offset, col_offset);
		return res;
	}

	template <uint DST_R, uint DST_C = DST_R, class T = float, uint SRC_R = DST_R, uint SRC_C = DST_C, int DST_Options = MatrixOptions(DST_R, DST_C, VKL_DEFAULT_MATRIX_ROW_MAJOR, VKL_DEFAULT_MATRIX_AUTO_ALIGN), int SRC_Options = MatrixOptions(SRC_R, SRC_C, VKL_DEFAULT_MATRIX_ROW_MAJOR, VKL_DEFAULT_MATRIX_AUTO_ALIGN)>
	static Matrix<T, DST_R, DST_C, DST_Options> ResizeMatrix(Matrix<T, SRC_R, SRC_C, SRC_Options> const& src, T const& diag = T(1))
	{
		return ConvertAndResizeMatrix<T, DST_R, DST_C, T, SRC_R, SRC_C, DST_Options, SRC_Options>(src, diag);
	}

	template <class T, uint R, uint C, int Options>
	static T const& GetCoeficient(Matrix<T, R, C, Options> const& m, uint row, uint col)
	{
		return m(row, col);
	}

	template <class T, uint R, uint C, int Options>
	static void SetCoeficient(Matrix<T, R, C, Options>& m, uint row, uint col, T const& value)
	{
		m(row, col) = value;
	}

	template <class T, uint R, uint C, int Options>
		requires (R == 1 || C == 1)
	static auto operator*(Matrix<T, R, C, Options> const& l, Matrix<T, R, C, Options> const& r)
	{
		return l.cwiseProduct(r);
	}

	template <class T, uint R, uint C, int Options>
	static auto Transpose(Matrix<T, R, C, Options> const& m)
	{
		return m.transpose();
	}

	template <class T, uint N, int Options>
	static auto Inverse(Matrix<T, N, N, Options> const& m)
	{
		return m.inverse();
	}

	template <class T, uint R, uint C, int Options>
		requires (R == 1 || C == 1)
	static auto InnerProduct(Matrix<T, R, C, Options> const& l, Matrix<T, R, C, Options> const& r)
	{
		return l.dot(r);
	}

	template <class T, uint R, uint C, int Options>
		requires (R == 1 || C == 1)
	static auto Dot(Matrix<T, R, C, Options> const& l, Matrix<T, R, C, Options> const& r)
	{
		return InnerProduct(l, r);
	}

	// 2D -> returns a scalar (magnitude of the cross vector)
	// 3D -> standard cross vector
	template <class T, uint R, uint C, int Options>
		requires ((R == 1 || C == 1) && (std::max(R, C) == 2 || std::max(R, C) == 3))
	static auto Cross(Matrix<T, R, C, Options> const& l, Matrix<T, R, C, Options> const& r)
	{
		return l.cross(r);
	}

	template <class T, uint R, uint C, int Options>
		requires (R == 1 || C == 1)
	static auto OuterProduct(Matrix<T, R, C, Options> const& l, Matrix<T, R, C, Options> const& r)
	{
		if constexpr (C == 1)
		{
			return l * r.transpose();
		}
		else
		{
			return l.transpose() * r;
		}
	}

	template <class T, uint R, uint C, int Options>
	static auto Normalize(Matrix<T, R, C, Options> const& v)
	{
		return v.normalized();
	}

	template <class T, uint R, uint C, int Options>
	static auto Length(Matrix<T, R, C, Options> const& v)
	{
		return v.norm();
	}

	template <class T, uint R, uint C, int Options>
	static auto Length2(Matrix<T, R, C, Options> const& v)
	{
		return v.squaredNorm();
	}

	template <class T, uint R, uint C, int Options>
	static auto Distance(Matrix<T, R, C, Options> const& a, Matrix<T, R, C, Options> const& b)
	{
		return Length(b - a);
	}

	template <class T, uint R, uint C, int Options>
	static auto Distance2(Matrix<T, R, C, Options> const& a, Matrix<T, R, C, Options> const& b)
	{
		return Length2(b - a);
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


