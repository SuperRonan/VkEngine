
template <typename SrcDerived, class Converter>
	requires 
	vkl::concepts::PlainCompileTimeSizedMatrix<Derived> &&
	vkl::concepts::CompileTimeSizedMatrixCompatible<SrcDerived> &&
	::vkl::concepts::Converter<Converter, Scalar, typename SrcDerived::Scalar>
static constexpr void InitFromOther(
	PlainObjectBase<Derived>& dst, 
	SrcDerived const& src, 
	typename Scalar const& diag = {}, 
	Converter const& converter = {}, 
	uint row_offset = 0, 
	uint col_offset = 0
) {
	using DST_T = Scalar;
	using SRC_T = typename SrcDerived::Scalar;
	constexpr const uint DST_R = RowsAtCompileTime;
	constexpr const uint DST_C = ColsAtCompileTime;
	constexpr const uint SRC_R = SrcDerived::RowsAtCompileTime;
	constexpr const uint SRC_C = SrcDerived::ColsAtCompileTime;

	// TODO implement with the offsets
	assert(row_offset == 0);
	assert(col_offset == 0);

	dst.setZero();
	// TODO check the efficiency of the loops
	for (uint i = 0; i < std::min(DST_R, SRC_R); ++i)
	{
		for (uint j = 0; j < std::min(DST_C, SRC_C); ++j)
		{
			dst.coeffRef(i, j) = converter(src(i, j));
		}
	}

	// Size of the square block top left 
	constexpr const uint SRC_square_N = std::min(SRC_R, SRC_C);
	constexpr const uint DST_square_N = std::min(DST_R, DST_C);

	// Fill the remaining diagonal
	if constexpr (DST_square_N > SRC_square_N)
	{
		//src.topLeftCorner(std::min(DST_R, SRC_R), std::min(DST_C, SRC_C))
		constexpr const uint N = DST_square_N - SRC_square_N;
		constexpr const uint O = SRC_square_N;
		for (uint i = 0; i < N; ++i)
		{
			dst.coeffRef(O + i, O + i) = diag;
		}
	}
}

template <
	class T0,
	class T1,
	vkl::concepts::CompileTimeSizedMatrixCompatible OtherMatrix
>
	requires ::vkl::concepts::Converter<T1, Scalar, typename OtherMatrix::Scalar> &&
	((OtherMatrix::RowsAtCompileTime != RowsAtCompileTime) || (OtherMatrix::ColsAtCompileTime != ColsAtCompileTime)) &&
	(std::convertible_to<typename OtherMatrix::Scalar, Scalar>)
EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE constexpr void _init2(OtherMatrix const& other, T1 const& converter)
{
	InitFromOther(this->derived(), other.derived(), Scalar(1), converter);
}

template <
	class T0,
	class T1,
	vkl::concepts::CompileTimeSizedMatrixCompatible OtherMatrix
>
	requires std::convertible_to<T1, Scalar> && 
	((OtherMatrix::RowsAtCompileTime != RowsAtCompileTime) || (OtherMatrix::ColsAtCompileTime != ColsAtCompileTime)) &&
	(std::convertible_to<typename OtherMatrix::Scalar, Scalar>)
EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE constexpr void _init2(OtherMatrix const& other, T1 const& diag)
{
	InitFromOther(this->derived(), other.derived(), diag, ::vkl::DefaultStaticCastConverter<Scalar, typename OtherMatrix::Scalar>{});
}

template <
	class T,
	vkl::concepts::CompileTimeSizedMatrixCompatible OtherMatrix
>
	requires 
	((OtherMatrix::RowsAtCompileTime != RowsAtCompileTime) || (OtherMatrix::ColsAtCompileTime != ColsAtCompileTime)) &&
	(std::convertible_to<typename OtherMatrix::Scalar, Scalar>)
EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE constexpr void _init1(OtherMatrix const& other)
{
	_init2<decltype(other), ::vkl::DefaultStaticCastConverter<Scalar, typename OtherMatrix::Scalar>>(other, ::vkl::DefaultStaticCastConverter<Scalar, typename OtherMatrix::Scalar>());
}
