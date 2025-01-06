
template <typename SrcDerived, class Converter>
	requires 
	((Derived::RowsAtCompileTime != Dynamic && Derived::ColsAtCompileTime != Dynamic) && (SrcDerived::RowsAtCompileTime != Dynamic && SrcDerived::ColsAtCompileTime != Dynamic)) && 
	::vkl::concepts::Converter<Converter, Scalar, typename SrcDerived::Scalar>
static constexpr void InitFromOther(
	PlainObjectBase<Derived>& dst, 
	PlainObjectBase<SrcDerived> const& src, 
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
		for (uint j = 0; j < std::max(DST_C, SRC_C); ++j)
		{
			dst.coeffRef(i, j) = converter(src.coeff(i, j));
		}
	}
	// Fill the remaining diagonal
	if ((DST_R > SRC_R) && (DST_C > SRC_C))
	{
		//src.topLeftCorner(std::min(DST_R, SRC_R), std::min(DST_C, SRC_C))
		constexpr const uint N = std::min(DST_R - SRC_C, DST_C - SRC_C);
		constexpr const uint O = std::min(DST_R, DST_C);
		for (uint i = 0; i < N; ++i)
		{
			dst.coeffRef(O + i, O + 1) = diag;
		}
	}
}

template <
	class T0,
	class T1,
	class OtherScalar, 
	int   OtherRows, 
	int   OtherCols, 
	int   OtherOptions, 
	int   OtherMaxRows, 
	int   OtherMaxCols
>
	requires ::vkl::concepts::Converter<T1, Scalar, OtherScalar>
EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE constexpr void _init2(Matrix<OtherScalar, OtherRows, OtherCols, OtherOptions, OtherMaxRows, OtherMaxCols> const& other, T1 const& converter)
{
	InitFromOther(this->derived(), other.derived(), Scalar{}, converter);
}

template <
	class T0,
	class T1,
	class OtherScalar, 
	int   OtherRows, 
	int   OtherCols, 
	int   OtherOptions, 
	int   OtherMaxRows, 
	int   OtherMaxCols
>
	requires std::convertible_to<T1, Scalar>
EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE constexpr void _init2(Matrix<OtherScalar, OtherRows, OtherCols, OtherOptions, OtherMaxRows, OtherMaxCols> const& other, T1 const& diag)
{
	InitFromOther(this->derived(), other.derived, diag, ::vkl::DefaultStaticCastConverter<Scalar, OtherScalar>{});
}

template <
	class T,
	class OtherScalar, 
	int   OtherRows, 
	int   OtherCols, 
	int   OtherOptions, 
	int   OtherMaxRows, 
	int   OtherMaxCols
>
EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE constexpr void _init1(Matrix<OtherScalar, OtherRows, OtherCols, OtherOptions, OtherMaxRows, OtherMaxCols> const& other)
{
	_init2<decltype(other), ::vkl::DefaultStaticCastConverter<Scalar, OtherScalar>>(other, ::vkl::DefaultStaticCastConverter<Scalar, OtherScalar>());
}
