
//template <::vkl::concepts::CompileTimeSizedMatrixCompatible<Matrix> OtherMatrix>
//	requires (((RowsAtCompileTime != OtherMatrix::RowsAtCompileTime) || (ColsAtCompileTime != OtherMatrix::ColsAtCompileTime)) && (std::convertible_to<typename OtherMatrix::Scalar, Scalar>))
//constexpr explicit Matrix(OtherMatrix const& other) 
//{
//	Base::template _init1<OtherMatrix>(other);
//}


template <::vkl::concepts::SameCompatibleMatrix<Matrix> Vec>
	requires ((RowsAtCompileTime == 1 || ColsAtCompileTime == 1))
constexpr Matrix& operator*=(Vec const& other)
{
	*this = (this->cwiseProduct(other));
	return *this;
}

constexpr Matrix& operator*=(Scalar const& s)
{
	(*this) = (*this * s);
	return *this;
}


template <::vkl::concepts::SameCompatibleMatrix<Matrix> Vec>
	requires ((RowsAtCompileTime == 1 || ColsAtCompileTime == 1))
constexpr Matrix& operator/=(Vec const& other)
{
	*this = (this->cwiseQuotient(other));
	return *this;
}

constexpr Matrix& operator/=(Scalar const& s)
{
	(*this) = (*this / s);
	return *this;
}

template <::vkl::concepts::SameCompatibleMatrix<Matrix> Mat>
constexpr Matrix& operator+=(Mat const& m)
{
	*this = (*this + m);
	return *this;
}

template <std::convertible_to<Scalar> OtherScalar>
constexpr Matrix& operator+=(OtherScalar o)
{
	(*this) = (*this + o);
	return *this;
}

template <::vkl::concepts::SameCompatibleMatrix<Matrix> Mat>
constexpr Matrix& operator-=(Mat const& m)
{
	*this = (*this - m);
	return *this;
}

template <std::convertible_to<Scalar> OtherScalar>
constexpr Matrix& operator-=(OtherScalar o)
{
	(*this) = (*this - o);
	return *this;
}


template <typename MatR>
	requires (::vkl::concepts::SameCompatibleMatrix<MatR, Matrix> && ((Matrix::RowsAtCompileTime + 1) == Matrix::ColsAtCompileTime) && std::derived_from<MatR, MatrixBase<MatR>>)
constexpr Matrix& operator*=(MatR const& r)
{
	(*this) = (*this * r);
	return *this;
}

template <::vkl::concepts::MatrixCompatible<Scalar, ColsAtCompileTime, ColsAtCompileTime> MatR>
	requires (std::derived_from<MatR, MatrixBase<MatR>>)
constexpr Matrix& operator*=(MatR const& r)
{
	(*this) = (*this * r);
	return *this;
}