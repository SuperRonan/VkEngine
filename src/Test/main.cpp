
#include <Core/DynamicValue.hpp>
#include <Core/VulkanCommons.hpp>
#include <Utils/stl_extension.hpp>



template <class T, Container<T> C>
const T* f(C const& c)
{
	return c.data();
}

int main(int argc, const char** argv)
{
	using namespace vkl;
	using namespace std::containers_operators;

	dv_<VkExtent3D> ex = makeZeroExtent3D();

	dv_<float> pi = 3.14f;

	dv_<int> pii = pi;

	dv_<double> tau = pi + pii;



	std::vector<std::string> a = { "a" };
	std::vector<std::string> bc = { "b", "c"};

	f<std::string>(a);

	std::vector<std::string> abc = a + bc;

	std::vector d = abc + "d"s;

	std::vector e = d + "e"s;

	return 0;
}

