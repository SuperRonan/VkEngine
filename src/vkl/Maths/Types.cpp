#include <vkl/Maths/Types.hpp>

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

		Vector4f a = w * u;

		Matrix4f tt(r);

		eg::Array<float, 4, 4> arrf(a);

		//arrf = a;
	}
}