
#include <Core/DynamicValue.hpp>
#include <Core/VulkanCommons.hpp>
#include <Utils/stl_extension.hpp>

int main(int argc, const char ** argv)
{
	using namespace vkl;
	using namespace std::vector_operators;

	dv_<VkExtent3D> e = makeZeroExtent3D();


	std::vector<std::string> a = { "a" };
	std::vector<std::string> bc = { "b", "c"};

	std::vector<std::string> abc = a + bc;


	return 0;
}

