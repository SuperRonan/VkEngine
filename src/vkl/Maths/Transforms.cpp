#include <vkl/Maths/Transforms.hpp>

namespace vkl
{
	void test()
	{
		using Scalar = float;
		Vector3<Scalar> Z(0, 1, 0);
		Vector3<Scalar> X(1, 0, 0);

		

		Matrix3<Scalar> m = BasisFrom2DBasisZX(Z, X);
	}
}