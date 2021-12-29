#include "Program.hpp"

namespace vkl
{
	void Program::createLayout()
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		VkDescriptorSetLayoutCreateInfo ci{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = bindings.size(),
			.pBindings = bindings.data(),
		};

		VK_CHECK(vkCreateDescriptorSetLayout)
	}
}