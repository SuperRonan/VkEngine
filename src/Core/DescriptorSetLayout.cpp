#include "DescriptorSetLayout.hpp"

namespace vkl
{

	void DescriptorSetLayout::create(VkDescriptorSetLayoutCreateInfo const& ci)
	{
		VK_CHECK(vkCreateDescriptorSetLayout(_app->device(), &ci, nullptr, &_handle), "Failed to create a descriptor set layout.");
	}

	void DescriptorSetLayout::destroy()
	{
		vkDestroyDescriptorSetLayout(_app->device(), _handle, nullptr);
		_handle = VK_NULL_HANDLE;
	}

	DescriptorSetLayout::DescriptorSetLayout(VkApplication* app, VkDescriptorSetLayoutCreateInfo const& ci) :
		VkObject(app)
	{
		create(ci);
	}
	
	DescriptorSetLayout::DescriptorSetLayout(VkApplication* app, std::vector<VkDescriptorSetLayoutBinding> const& bindings, std::vector<std::string> const& names) :
		VkObject(app),
		_names(names)
	{
		VkDescriptorSetLayoutCreateInfo ci = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.bindingCount = (uint32_t)bindings.size(),
			.pBindings = bindings.data(),
		};
		_bindings = bindings;
		create(ci);
	}

	DescriptorSetLayout::~DescriptorSetLayout()
	{
		if (_handle)
		{
			destroy();
		}
	}

}