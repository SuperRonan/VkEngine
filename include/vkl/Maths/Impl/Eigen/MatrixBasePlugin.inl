inline Scalar at(uint i, uint j) const { return this->operator()(i, j); }
inline Scalar& at(uint i, uint j) { return this->operator()(i, j); }
inline Scalar at(uint i) const { return this->operator[](i); }
inline Scalar& at(uint i) { return this->operator[](i); }

inline RealScalar squaredLength() const { return squaredNorm(); } 
inline RealScalar length() const { return norm(); }
inline RealScalar invLength(void) const { return fast_inv_sqrt(squaredNorm()); }

template<typename OtherDerived>
inline Scalar squaredDistanceTo(const MatrixBase<OtherDerived>& other) const
{
	return (derived() - other.derived()).squaredNorm();
}

template<typename OtherDerived>
inline RealScalar distanceTo(const MatrixBase<OtherDerived>& other) const
{
	return sqrt(derived().squaredDistanceTo(other));
}

inline void scaleTo(RealScalar l) { RealScalar vl = norm(); if (vl > 1e-9) derived() *= (l / vl); }

inline Transpose<Derived> transposed() { return this->transpose(); }
inline const Transpose<Derived> transposed() const { return this->transpose(); }

inline uint minComponentId(void) const { int i; this->minCoeff(&i); return i; }
inline uint maxComponentId(void) const { int i; this->maxCoeff(&i); return i; }

template<typename OtherDerived>
void makeFloor(const MatrixBase<OtherDerived>& other) { derived() = derived().cwiseMin(other.derived()); }
template<typename OtherDerived>
void makeCeil(const MatrixBase<OtherDerived>& other) { derived() = derived().cwiseMax(other.derived()); }

template <::vkl::concepts::SameCompatibleMatrix<Derived> Vec>
	requires ((RowsAtCompileTime == 1 || ColsAtCompileTime == 1))
constexpr auto operator*(Vec const& other) const
{
	return this->cwiseProduct(other);
}

template <::vkl::concepts::SameCompatibleMatrix<Derived> Vec>
	requires ((RowsAtCompileTime == 1 || ColsAtCompileTime == 1))
constexpr auto operator/(Vec const& other) const
{
	return this->cwiseQuotient(other);
}

template <std::convertible_to<typename Derived::Scalar> OtherScalar>
constexpr auto operator+(OtherScalar o) const
{
	return ((*this) + Derived::Constant(o)).eval();
}

template <std::convertible_to<typename Derived::Scalar> OtherScalar>
constexpr auto operator-(OtherScalar o) const
{
	return ((*this) - Derived::Constant(o)).eval();
}

// AffineXForm = AffineXForm * AffineXForm 
template <typename MatR>
	requires (::vkl::concepts::SameCompatibleMatrix<MatR, Derived> && ((Derived::RowsAtCompileTime + 1) == Derived::ColsAtCompileTime) && std::derived_from<MatR, MatrixBase<MatR>>)
constexpr auto operator*(MatR const& r) const
{
	constexpr uint N = Derived::RowsAtCompileTime;
#if 0
	using EVRT = typename std::remove_cvref<typename Derived::EvalReturnType>::type;
	Matrix<typename Derived::Scalar, N, N + 1, EVRT::Options> res = res.Constant(Scalar(0));
	const auto Ql = this->block(0, 0, N, N);
	const auto Qr = r.block(0, 0, N, N);
	decltype(auto) Q = res.block(0, 0, N, N);
	Q = Ql * Qr;
	const auto Tr = this->block(0, N, N - 1, 1);
	const auto Tl = r.block(0, N, N - 1, 1);
	decltype(auto) t = res.block(0, N, N - 1, 1);
	t = Ql * Tr + Tl;
	return res;
#else
	using EVRT_R = typename std::remove_cvref<typename MatR::EvalReturnType>::type;
	using MatN = Matrix<typename MatR::Scalar, N + 1, N + 1, EVRT_R::Options>;
	MatN R_N = MatN(r);	
	auto res = ((*this) * R_N).eval();
	return res;
#endif
}