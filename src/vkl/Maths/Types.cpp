#include <vkl/Maths/Types.hpp>

#include <vkl/Maths/AffineXForm.hpp>

namespace vkl
{
	void test()
	{
		Vector4f u, v;

		InnerProduct(u, v);
		Matrix4f w = OuterProduct(u, v);

		Matrix3f r = Matrix3f::Random();

		Matrix4f t = ResizeMatrix<4>(r);

		eg::Matrix4i i = ConvertAndResizeMatrix<int, 4>(r);

		DiagonalMatrix(u, 1.0f);

		u * v;
		u *= v;
		u * 2;
		u *= 2;

		u / 3;
		u /= 3;

		Vector4f a = w * u;
		Vector4f::Options;

		Matrix4f tt(r);

		eg::Array<float, 4, 4> arrf(a);

		Matrix3x4f xform = DiagonalMatrix<3, 4>(1.0f);

		Matrix3x4f uv_mat = MakeFromRows(u, v, a);
		//Matrix4x3f uv_mat2 = MakeFromCols(u, v, a);
		Vector4f::Options;

		auto ttt = u.transpose();
		using TTT = decltype(ttt);
		static_assert(concepts::CompileTimeSizedRowVectorCompatible<TTT>);
		static_assert(!concepts::CompileTimeSizedColVectorCompatible<TTT>);
		constexpr const bool test_concept = concepts::CompileTimeSizedColVectorCompatible<int>;

		decltype(xform.row(0))::EvalReturnType;
		
		xform.row(0).IsPlainObjectBase;
		xform.IsPlainObjectBase;
		using X = decltype(ttt.transpose());

		Matrix3x4f yform = Matrix3x4f::Random();
		auto zform = xform + yform;
		decltype(zform)::Nested;
		Vector4f::EvalReturnType;

		using Traits = impl::VectorPackTraits<Vector4f, Vector4f>;
		Traits::Scalar x = 12;

		SetColumn(xform, 0, Vector3f::Random());

		using A = int;
		using B = const int &;
		constexpr const bool same_AB = std::same_as<typename std::remove_cvref<A>::type, typename std::remove_cvref<B>::type>;
		
		Vector4f::SizeAtCompileTime;

		//constexpr const bool derived = std::derived_from<Vector4f, eg::EigenBase<Vector4f>>;
		constexpr const bool derived = std::derived_from<int, eg::EigenBase<int>>;
		u.stableNormalized();
		u.norm();

		std::same_as<int, int>;
		auto prod = xform * t;
		auto doot = Dot(u, v);

		arrf + 0.5f;
		u + 0.5f;

		u += 0.04;
		
		Vector4f::Constant(0);
		auto comp = u == v;
		
		Lerp(u, v, 12);

		u += v;

		Vector3f p = Vector3f(u);

		Matrix4f qq = Matrix4f(TranslationMatrix(Vector3f::Random().eval())) * Matrix4f(DiagonalMatrixV(Vector3f::Random().eval())) * tt;

		using Prod = Eigen::Product<Matrix4f, Matrix4f, 0>;
		using Prod_t = Prod::PlainObject;
		Eigen::Matrix<float, 4, 4>;
		Eigen::internal::traits<Prod>::Flags;

		auto p3 = (Matrix4f(Matrix4f::Random()) * Matrix4f(Matrix4f::Random()) * Matrix4f(Matrix4f::Random())).eval();

		using Eval = Eigen::internal::evaluator<Eigen::Matrix<float, 4, 4, 0, 4, 4>>;

		Eigen::PlainObjectBase<Eigen::Matrix<float, 4, 4, 0, 4, 4>> *plain_object_base = nullptr;
	}
}