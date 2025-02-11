#include <vkl/Maths/Transforms.hpp>

#include <vkl/Maths/AffineXForm.hpp>

namespace vkl
{
	void test()
	{
		using Scalar = float;
		Vector3<Scalar> Z(0, 1, 0);
		Vector3<Scalar> X(1, 0, 0);

		

		Matrix3<Scalar> m = BasisFrom2DBasisZX(Z, X);

		Matrix3f diag = Matrix3f(TranslationMatrix(Vector2f(2, 3)));
		

		Matrix2f m2 = Matrix2f::Random();
		Matrix3f m3 = Matrix3f(Matrix2f::Random());

		auto mm = diag * m3;

		Vector3f v = v.Constant(0);

		v += 1.0f;

		AffineXForm3Df l = AffineXForm3Df::Random();
		AffineXForm3Df r = AffineXForm3Df::Random();

		AffineXForm3Df prod = l * r;

		prod *= r;

		l *= Matrix4f(m);

		Z * X;
	}
}