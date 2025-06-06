#pragma once

#define EIGEN_DEFAULT_TO_ROW_MAJOR
#define EIGEN_HAS_CXX17_IFCONSTEXPR

#ifndef VKL_DEFAULT_MATRIX_ROW_MAJOR
#define VKL_DEFAULT_MATRIX_ROW_MAJOR true
#endif

#ifndef VKL_DEFAULT_MATRIX_AUTO_ALIGN
#define VKL_DEFAULT_MATRIX_AUTO_ALIGN true
#endif

#if VKL_BUILD_FAST_DEBUG
#define EIGEN_NO_DEBUG 1
#endif

#include <that/core/BasicTypes.hpp>

#include <type_traits>

#include <cassert>

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

namespace vkl
{
	static constexpr int Eigen_Dynamic = -1;

	namespace concepts
	{
		// These matrices concepts also work for eg::Array
		template <class C>
		concept PlainCompileTimeSizedMatrix = (C::IsPlainObjectBase != 0) && (C::RowsAtCompileTime != Eigen_Dynamic) && (C::ColsAtCompileTime != Eigen_Dynamic);

		template <class C>
		concept PlainCompileTimeSizedSquareMatrix = PlainCompileTimeSizedMatrix<C> && (C::RowsAtCompileTime == C::ColsAtCompileTime);

#define DECLARE_NESTED_CONCEPT(CONCEPT) \
		template <class C> \
		concept EvalTo##CONCEPT = ((C::IsPlainObjectBase == 0) && Plain##CONCEPT<typename C::EvalReturnType>); \
		template <class C> \
		concept CONCEPT##Compatible = Plain##CONCEPT<C> || EvalTo##CONCEPT <C>; 

#define DECLARE_NESTED_CONCEPT_RC(CONCEPT) \
		template <class Candidate, int R, int C> \
		concept EvalTo##CONCEPT = ((Candidate::IsPlainObjectBase == 0) && Plain##CONCEPT<typename Candidate::EvalReturnType, R, C>); \
		template <class Candidate, int R, int C> \
		concept CONCEPT##Compatible = Plain##CONCEPT<Candidate, R, C> || EvalTo##CONCEPT <Candidate, R, C>; 

#define DECLARE_NESTED_CONCEPT_N(CONCEPT) \
		template <class Candidate, int N> \
		concept EvalTo##CONCEPT = ((Candidate::IsPlainObjectBase == 0) && Plain##CONCEPT<typename Candidate::EvalReturnType, N>); \
		template <class Candidate, int N> \
		concept CONCEPT##Compatible = Plain##CONCEPT<Candidate, N> || EvalTo##CONCEPT <Candidate, N>; 


		DECLARE_NESTED_CONCEPT(CompileTimeSizedMatrix)
		DECLARE_NESTED_CONCEPT(CompileTimeSizedSquareMatrix)

		template <class C>
		concept PlainCompileTimeSizedVector = PlainCompileTimeSizedMatrix<C> && requires(C const& c)
		{
			C::RowsAtCompileTime == 1 || C::ColsAtCompileTime == 1;
		};

		DECLARE_NESTED_CONCEPT(CompileTimeSizedVector)

		template <class C>
		concept PlainCompileTimeSizedRowVector = PlainCompileTimeSizedMatrix<C> && (C::RowsAtCompileTime == 1);

		template <class C>
		concept PlainCompileTimeSizedColVector = PlainCompileTimeSizedMatrix<C> && (C::ColsAtCompileTime == 1);

		DECLARE_NESTED_CONCEPT(CompileTimeSizedRowVector)
		DECLARE_NESTED_CONCEPT(CompileTimeSizedColVector)

		template <class Candidate, class Scalar>
		concept CompatibleScalar = std::convertible_to<typename Candidate::Scalar, Scalar>;

		//template <class Candidate, class Scalar>
		//concept SameScalar = std::same_as<typename Candidate::Scalar, typename std::remove_cvref_t<Scalar>>;
		
		template <class Candidate, int R, int C>
		concept PlainMatrixAny = (Candidate::IsPlainObjectBase != 0) && (Candidate::RowsAtCompileTime == R) && (Candidate::ColsAtCompileTime == C);

		template <class Candidate, int N>
		concept PlainSquareMatrixAny = PlainMatrixAny<Candidate, N, N>;

		DECLARE_NESTED_CONCEPT_RC(MatrixAny)
		DECLARE_NESTED_CONCEPT_N(SquareMatrixAny)

		template <class Candidate, int N>
		concept PlainColVectorAny = PlainMatrixAny<Candidate, N, 1>;
		template <class Candidate, int N> 
		concept PlainRowVectorAny = PlainMatrixAny<Candidate, 1, N>;

		DECLARE_NESTED_CONCEPT_N(ColVectorAny)
		DECLARE_NESTED_CONCEPT_N(RowVectorAny)

		template <class Candidate, class Scalar, int R, int C>
		concept MatrixCompatible = MatrixAnyCompatible<Candidate, R, C> && CompatibleScalar<Candidate, Scalar>;

		template <class Candidate, class Scalar, int N>
		concept SquareMatrixCompatible = MatrixCompatible<Candidate, Scalar, N, N>;

		template <class Candidate, class Scalar, int N>
		concept ColVectorCompatible = ColVectorAnyCompatible<Candidate, N> && CompatibleScalar<Candidate, Scalar>;

		template <class Candidate, class Scalar, int N>
		concept RowVectorCompatible = RowVectorAnyCompatible<Candidate, N>&& CompatibleScalar<Candidate, Scalar>;

		template <class MatR, class MatL>
		concept SameCompatibleMatrix = ((MatL::RowsAtCompileTime == MatR::RowsAtCompileTime) && (MatL::ColsAtCompileTime == MatR::ColsAtCompileTime) && (std::is_same<typename std::remove_cvref_t<MatL>::Scalar, typename std::remove_cvref_t<MatR>::Scalar>::value));
	}
}

#define EIGEN_MATRIX_PLUGIN <vkl/Maths/Impl/Eigen/MatrixPlugin.inl>
#define EIGEN_MATRIXBASE_PLUGIN <vkl/Maths/Impl/Eigen/MatrixBasePlugin.inl>
#define EIGEN_PLAINOBJECTBASE_PLUGIN <vkl/Maths/Impl/Eigen/PlainObjectBasePlugin.inl>
#define EIGEN_DENSEBASE_PLUGIN <vkl/Maths/Impl/Eigen/DenseBasePlugin.inl>

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

	static constexpr int MatrixOptions(int R, int C, bool row_major, bool auto_align)
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

	static constexpr int GetBestOptions(int R, int C, int src_options)
	{
		const bool row_major = (src_options & eg::RowMajor) == eg::RowMajor;
		const bool auto_align = (src_options & eg::DontAlign) != eg::DontAlign;
		return MatrixOptions(R, C, row_major, auto_align);
	}

	static constexpr bool ExtractRowMajorFromMatrixOptions(int options)
	{
		return (options & eg::RowMajor) != 0;
	}

	static constexpr bool ExtractAutoAlignFromMatrixOptions(int options)
	{
		return (options & eg::DontAlign) != eg::DontAlign;
	}

	// Follow the math convention
	// Rows x Cols
	// Direct template alias, can be used for template param deduction
	template <class T, int R, int C = R, int Options = MatrixOptions(R, C, VKL_DEFAULT_MATRIX_ROW_MAJOR, VKL_DEFAULT_MATRIX_AUTO_ALIGN)>
	using Matrix = eg::Matrix<T, R, C, Options>;

	template <class T, int N, int Options = MatrixOptions(false, VKL_DEFAULT_MATRIX_AUTO_ALIGN)>
		requires ((Options & eg::RowMajor) == eg::ColMajor)
	using ColVector = Matrix<T, N, 1, Options>;

	template <class T, int N, int Options = MatrixOptions(true, VKL_DEFAULT_MATRIX_AUTO_ALIGN)>
		requires ((Options & eg::RowMajor) == eg::RowMajor)
	using RowVector = Matrix<T, 1, N, Options>;

	template <class T, int N, int Options = MatrixOptions(false, VKL_DEFAULT_MATRIX_AUTO_ALIGN)>
	using Vector = ColVector<T, N, Options>;

	template <class Derived>
		requires (std::derived_from<Derived, eg::EigenBase<Derived>>)
	static constexpr int GetTypeOptions()
	{
		using EVRT = typename std::remove_cvref<typename std::remove_cvref_t<Derived>::EvalReturnType>::type;
		return EVRT::Options;
	}

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

	using Vector2i = Vector2<int>;
	using Vector3i = Vector3<int>;
	using Vector4i = Vector4<int>;

	using Vector2u = Vector2<uint>;
	using Vector3u = Vector3<uint>;
	using Vector4u = Vector4<uint>;

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

	template <int R, int C>
	using Matrixf = Matrix<float, R, C>;
	template <int R, int C>
	using Matrixd = Matrix<double, R, C>;
	template <int R, int C>
	using Matrixi = Matrix<int, R, C>;
	template <int R, int C>
	using Matrixu = Matrix<uint, R, C>;

	using Matrix2f = Matrix2<float>;
	using Matrix3f = Matrix3<float>;
	using Matrix4f = Matrix4<float>;
	using Matrix4x3f = Matrix4x3<float>;
	using Matrix3x4f = Matrix3x4<float>;

	using Matrix2d = Matrix2<double>;
	using Matrix3d = Matrix3<double>;
	using Matrix4d = Matrix4<double>;

	template <class T, int R, int C = R>
	using MatrixRowMajor = Matrix<T, R, C, MatrixOptions(R, C, true, VKL_DEFAULT_MATRIX_AUTO_ALIGN)>;

	template <class T, int R, int C = R>
	using MatrixColMajor = Matrix<T, R, C, MatrixOptions(R, C, false, VKL_DEFAULT_MATRIX_AUTO_ALIGN)>;

	template <class T>
	using Matrix3x4RowMajor = MatrixRowMajor<T, 3, 4>;

	using Matrix3x4fRowMajor = Matrix3x4RowMajor<float>;

	template <int R, int C = R, class Scalar = float, int Options = MatrixOptions(R, C, VKL_DEFAULT_MATRIX_ROW_MAJOR, VKL_DEFAULT_MATRIX_AUTO_ALIGN)>
	static constexpr Matrix<Scalar, R, C, Options> MakeUniformMatrix(Scalar const& value)
	{
		return Matrix<Scalar, R, C, Options>::Constant(value);
	}

	template <int N, class Scalar = float, bool auto_align = VKL_DEFAULT_MATRIX_AUTO_ALIGN>
	static constexpr auto MakeUniformColVector(Scalar const& value)
	{
		return MakeUniformMatrix<N, 1, Scalar, MatrixOptions(false, auto_align)>(value);
	}

	template <int N, class Scalar = float, bool auto_align = VKL_DEFAULT_MATRIX_AUTO_ALIGN>
	static constexpr auto MakeUniformRowVector(Scalar const& value)
	{
		return MakeUniformMatrix<1, N, Scalar, MatrixOptions(true, auto_align)>(value);
	}

	template <int N, class Scalar = float, bool auto_align = VKL_DEFAULT_MATRIX_AUTO_ALIGN>
	static constexpr auto MakeUniformVector(Scalar const& value)
	{
		return MakeUniformColVector<N, Scalar, auto_align>(value);
	}

	template <concepts::CompileTimeSizedMatrixCompatible Mat>
	static constexpr auto GetRow(Mat const& m, uint i)
	{
		return m.row(i);
	}

	template <concepts::CompileTimeSizedMatrixCompatible Mat>
	static constexpr auto GetColumn(Mat const& m, uint i)
	{
		return m.col(i);
	}

	template <class T, int R, int C, int Options, concepts::RowVectorCompatible<T, C> RowVector>
	static constexpr void SetRow(Matrix<T, R, C, Options>& m, uint i, RowVector const& v)
	{
		m.row(i) = v;
	}

	template <class T, int R, int C, int Options, concepts::ColVectorCompatible<T, R> ColVector>
	static constexpr void SetColumn(Matrix<T, R, C, Options>& m, uint i, ColVector const& v)
	{
		m.col(i) = v;
	}

	template<class T, int R, int C, int Options>
	static constexpr auto MakeFromRows(std::array<RowVector<T, C, Options>, R> const& rows)
	{
		Matrix<T, R, C, MatrixOptions(R, C, VKL_DEFAULT_MATRIX_ROW_MAJOR, ExtractAutoAlignFromMatrixOptions(Options))> res;
		for (uint i = 0; i < R; ++i)
		{
			SetRow(res, i, rows[i]);
		}
		return res;
	}

	template<class T, int R, int C, int Options>
	static constexpr auto MakeFromCols(std::array<ColVector<T, R, Options>, C> const& columns)
	{
		Matrix<T, R, C, MatrixOptions(R, C, VKL_DEFAULT_MATRIX_ROW_MAJOR, ExtractAutoAlignFromMatrixOptions(Options))> res;
		for (uint i = 0; i < C; ++i)
		{
			SetColumn(res, i, columns[i]);
		}
		return res;
	}

	namespace impl
	{	
		template <concepts::CompileTimeSizedVectorCompatible ... Args>
		struct VectorPackTraits
		{
			//template <concepts::CompileTimeSizedVector Vec>
			//static constexpr int GetColsAtCompileTime() noexcept
			//{
			//	return Vec::ColsAtCompileTime;
			//}

			//template <concepts::CompileTimeSizedVector Vec>
			//static constexpr int GetRowsAtCompileTime() noexcept
			//{
			//	return Vec::RowsAtCompileTime;
			//}

			static constexpr const size_t Count = sizeof...(Args);
			static constexpr const int MaxCols = std::max(std::initializer_list<size_t>{Args::ColsAtCompileTime...});
			static constexpr const int MaxRows = std::max(std::initializer_list<size_t>{Args::RowsAtCompileTime...});
			using AsTuple = std::tuple<Args...>;
			using FirstVectorType_ = typename std::tuple_element<0, AsTuple>::type;
			using FirstVectorType = typename std::remove_cvref<typename FirstVectorType_::EvalReturnType>::type;
			using Scalar = typename FirstVectorType::Scalar;
			static constexpr const int Options = FirstVectorType::Options;
		};
	}

	template <concepts::CompileTimeSizedRowVectorCompatible ... Rows>
	static constexpr auto MakeFromRows(Rows... rows)
	{
		using Traits = impl::VectorPackTraits<Rows...>;
		const int Options = eg::RowMajor | Traits::Options;
		std::array<RowVector<typename Traits::Scalar, Traits::MaxCols, Options>, Traits::Count> as_array = {rows...};
		return MakeFromRows<typename Traits::Scalar, Traits::Count, Traits::MaxCols, Options>(as_array);
	}

	template <concepts::CompileTimeSizedColVectorCompatible ... Cols>
	static constexpr auto MakeFromCols(Cols... cols)
	{
		using Traits = impl::VectorPackTraits<Cols...>;
		const int Options = ~eg::RowMajor & Traits::Options;
		std::array<ColVector<typename Traits::Scalar, Traits::MaxRows, Options>, Traits::Count> as_array = { cols... };
		return MakeFromCols<typename Traits::Scalar, Traits::MaxRows, Traits::Count, Options>(as_array);
	}

	// transposes
	template <concepts::CompileTimeSizedColVectorCompatible ... Cols>
	static constexpr auto MakeFromRows(Cols... rows)
	{
		using Traits = impl::VectorPackTraits<Cols...>;
		const int Options = eg::RowMajor | Traits::Options;
		std::array<RowVector<typename Traits::Scalar, Traits::MaxRows, Options>, Traits::Count> as_array = { rows.transpose()...};
		return MakeFromRows<typename Traits::Scalar, Traits::Count, Traits::MaxRows, Options>(as_array);
	}

	// transposes
	template <concepts::CompileTimeSizedRowVectorCompatible ... Rows>
	static constexpr auto MakeFromCols(Rows... cols)
	{
		using Traits = impl::VectorPackTraits<Rows...>;
		const int Options = ~eg::RowMajor & Traits::Options;
		std::array<ColVector<typename Traits::Scalar, Traits::MaxRows, Options>, Traits::Count> as_array = { cols.transpose()...};
		return MakeFromCols<typename Traits::Scalar, Traits::MaxRows, Traits::Count, Options>(as_array);
	}

	// Make a diagonal matrix 
	// Returns the Identity matrix by default
	// The matrix is square by default, but is can be rectangular
	template <class T, int R, int C = R, int Options = MatrixOptions(R, C, VKL_DEFAULT_MATRIX_ROW_MAJOR, VKL_DEFAULT_MATRIX_AUTO_ALIGN)>
	static constexpr Matrix<T, R, C, Options> MakeMatrix(T const& t = T(1))
	{
		Matrix<T, R, C, Options> res = Matrix<T, R, C, Options>::Zero();
		for (uint i = 0; i < std::min(R, C); ++i)
		{
			res(i, i) = t;
		}
		return res;
	}

	template <int R, int C = R, class Scalar = float, int Options = MatrixOptions(R, C, VKL_DEFAULT_MATRIX_ROW_MAJOR, VKL_DEFAULT_MATRIX_AUTO_ALIGN)>
	static constexpr Matrix<Scalar, R, C, Options> DiagonalMatrix(Scalar const& d = Scalar(1))
	{
		return MakeMatrix<Scalar, R, C, Options>(d);
	}

	// Make a square diagonal matrix with the vector coefs in the diagonal
	template <concepts::CompileTimeSizedVectorCompatible Vec>
	static constexpr auto MakeMatrixV(Vec const& diag)
	{
		constexpr const int N = std::max(Vec::RowsAtCompileTime, Vec::ColsAtCompileTime);
		using Scalar = typename Vec::Scalar;
		constexpr const int Options = GetTypeOptions<Vec>();
		using Res_t = Matrix<Scalar, N, N, Options>;
		Res_t res = Res_t::Zero();
		for (uint i = 0; i < N; ++i)
		{
			res(i, i) = diag[i];
		}
		return res;
	}

	template <concepts::CompileTimeSizedVectorCompatible Vec>
	static constexpr auto DiagonalMatrixV(Vec const& diag)
	{
		return MakeMatrixV(diag);
	}

	template <concepts::CompileTimeSizedVectorCompatible Vec>
	static constexpr auto DiagonalMatrixV(Vec const& diag, typename Vec::Scalar const& last)
	{
		Vector<typename Vec::Scalar, std::max(Vec::ColsAtCompileTime, Vec::RowsAtCompileTime), GetTypeOptions<Vec>()> dd;
		dd << diag , last;
		return DiagonalMatrixV(dd);
	}

	template <concepts::CompileTimeSizedVectorCompatible Vec>
	static constexpr auto DiagonalMatrix(Vec const& diag, typename Vec::Scalar const& last)
	{
		return DiagonalMatrixV(diag, last);
	}

	template <concepts::CompileTimeSizedMatrixCompatible Mat>
	static constexpr auto ExtractDiagonalVector(Mat const& m)
	{
		return m.diagonal();
	}

	template <int R, int C = R, concepts::CompileTimeSizedMatrixCompatible Mat>
	static constexpr auto ExtractBlock(Mat const& m, uint row_offset = 0, uint col_offset = 0)
	{
		return m.block<R, C>(row_offset, col_offset);
	}

	template <concepts::PlainCompileTimeSizedMatrix DstMat, concepts::CompileTimeSizedMatrixCompatible SrcMat>
	static constexpr void SetBlock(DstMat& dst, uint row_offset, uint col_offset, SrcMat const& src)
	{
		dst.block<SrcMat::RowsAtCompileTime, SrcMat::ColsAtCompileTime>(row_offset, col_offset) = src;
	}

	template <concepts::PlainCompileTimeSizedMatrix DstMat, concepts::CompileTimeSizedMatrixCompatible SrcMat>
	static constexpr void SetBlock(DstMat& dst, SrcMat const& src)
	{
		SetBlock(dst, 0, 0, src);
	}
	

	template <
		class DST_T, 
		int DST_R, 
		int DST_C = DST_R, 
		class SRC_T = DST_T, 
		int SRC_R = DST_R, 
		int SRC_C = DST_C, 
		int DST_Options = MatrixOptions(DST_R, DST_C, VKL_DEFAULT_MATRIX_ROW_MAJOR, VKL_DEFAULT_MATRIX_AUTO_ALIGN), 
		int SRC_Options = MatrixOptions(SRC_R, SRC_C, VKL_DEFAULT_MATRIX_ROW_MAJOR, VKL_DEFAULT_MATRIX_AUTO_ALIGN), 
		concepts::Converter<DST_T, SRC_T> Converter = DefaultStaticCastConverter<DST_T, SRC_T>
	>
	static constexpr Matrix<DST_T, DST_R, DST_C, DST_Options> ConvertAndResizeMatrix(Matrix<SRC_T, SRC_R, SRC_C, SRC_Options> const& src, DST_T const& diag = DST_T(1), Converter const& converter = Converter(), uint row_offset = 0, uint col_offset = 0)
	{
		using Res_t = Matrix<DST_T, DST_R, DST_C, DST_Options>;
		Res_t res;
		Res_t::InitFromOther(res, src, diag, converter, row_offset, col_offset);
		return res;
	}

	template <int R, int C = R, concepts::CompileTimeSizedMatrixCompatible SrcMatrix>
	static constexpr auto ResizeMatrix(SrcMatrix const& src)
	{
		using Ret_t = Matrix<typename SrcMatrix::Scalar, R, C>;
		return Ret_t(src);
	}

	template <concepts::CompileTimeSizedMatrixCompatible Mat>
	static constexpr decltype(auto) GetCoeficient(Mat const& m, uint row, uint col)
	{
		return m(row, col);
	}

	template <class T, int R, int C, int Options>
	static constexpr void SetCoeficient(Matrix<T, R, C, Options>& m, uint row, uint col, T const& value)
	{
		m(row, col) = value;
	}

	//template <concepts::CompileTimeSizedVectorCompatible VecL, concepts::CompileTimeSizedVectorCompatible VecR>
	//	requires concepts::SameCompatibleMatrix<VecL, VecR>
	//static constexpr auto operator*(VecL const& l, VecR const& r)
	//{
	//	return l.cwiseProduct(r);
	//}

	template <concepts::CompileTimeSizedMatrixCompatible Mat>
	static constexpr auto Transpose(Mat const& m)
	{
		return m.transpose();
	}

	template <concepts::CompileTimeSizedMatrixCompatible Mat>
		requires (Mat::RowsAtCompileTime == Mat::ColsAtCompileTime)
	static constexpr auto Inverse(Mat const& m)
	{
		return m.inverse();
	}

	template <concepts::CompileTimeSizedVectorCompatible VecL, concepts::CompileTimeSizedVectorCompatible VecR>
		requires concepts::SameCompatibleMatrix<VecL, VecR>
	static constexpr auto InnerProduct(VecL const& l, VecR const& r)
	{
		return l.dot(r);
	}

	template <concepts::CompileTimeSizedVectorCompatible VecL, concepts::CompileTimeSizedVectorCompatible VecR>
		requires concepts::SameCompatibleMatrix<VecL, VecR>
	static constexpr auto Dot(VecL const& l, VecR const& r)
	{
		return InnerProduct(l, r);
	}

	// 2D -> returns a scalar (magnitude of the cross vector)
	// 3D -> standard cross vector
	template <concepts::CompileTimeSizedVectorCompatible VecL, concepts::CompileTimeSizedVectorCompatible VecR>
		requires concepts::SameCompatibleMatrix<VecL, VecR> &&
		(std::max(VecL::RowsAtCompileTime, VecL::ColsAtCompileTime) == 2 || std::max(VecL::RowsAtCompileTime, VecL::ColsAtCompileTime) == 3)
	static auto Cross(VecL const& l, VecR const& r)
	{
		return l.cross(r);
	}

	template <concepts::CompileTimeSizedVectorCompatible VecL, concepts::CompileTimeSizedVectorCompatible VecR>
		requires concepts::SameCompatibleMatrix<VecL, VecR>
	static auto OuterProduct(VecL const& l, VecR const& r)
	{
		if constexpr (VecL::ColsAtCompileTime == 1)
		{
			return l * r.transpose();
		}
		else
		{
			return l.transpose() * r;
		}
	}

	template <concepts::CompileTimeSizedVectorCompatible Vec>
	static constexpr auto Normalize(Vec const& v)
	{
		return v.normalized();
	}

	template <concepts::CompileTimeSizedVectorCompatible Vec>
	static constexpr auto SafeNormalize(Vec const& v)
	{
		return v.stableNormalized();
	}

	template <concepts::CompileTimeSizedMatrixCompatible Mat>
	static constexpr auto Length(Mat const& v)
	{
		return v.norm();
	}

	template <concepts::CompileTimeSizedMatrixCompatible Mat>
	static constexpr auto Length2(Mat const& v)
	{
		return v.squaredNorm();
	}

	template <concepts::CompileTimeSizedMatrixCompatible MatA, concepts::CompileTimeSizedMatrixCompatible MatB>
		requires concepts::SameCompatibleMatrix<MatA, MatB>
	static constexpr auto Distance(MatA const& a, MatB const& b)
	{
		return Length(b - a);
	}

	template <concepts::CompileTimeSizedMatrixCompatible MatA, concepts::CompileTimeSizedMatrixCompatible MatB>
		requires concepts::SameCompatibleMatrix<MatA, MatB>
	static constexpr auto Distance2(MatA const& a, MatB const& b)
	{
		return Length2(b - a);
	}

	template <concepts::CompileTimeSizedMatrixCompatible Mat>
	static constexpr auto Sqrt(Mat const& mat)
	{
		return mat.cwiseSqrt();
	}

	template <concepts::CompileTimeSizedMatrixCompatible Vec>
	static constexpr auto Sum(Vec const& vec)
	{
		return vec.sum();
	}

	template <concepts::CompileTimeSizedMatrixCompatible Vec>
	static constexpr auto Average(Vec const& vec)
	{
		return vec.mean();
	}

	template <concepts::CompileTimeSizedMatrixCompatible Vec>
	static constexpr auto Prod(Vec const& vec)
	{
		return vec.prod();
	}

	template <concepts::CompileTimeSizedMatrixCompatible MatL, concepts::SameCompatibleMatrix<MatL> MatR>
	static constexpr auto Lerp(MatL const& l, MatR const& r, typename MatL::Scalar const& t)
	{
		return r * t + (typename MatL::Scalar(1) - t) * l;
	}
	

	template <class T>
	static void Swap(T& a, T& b)
	{
		std::swap(a, b);
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


