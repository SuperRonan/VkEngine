#include "DescriptorPool.hpp"

namespace vkl
{
	DescriptorPool::DescriptorPool(CreateInfo const& ci):
		VkObject(ci.app, ci.name),
		_layout(ci.layout),
		_max_sets(ci.max_sets),
		_flags(ci.flags)
	{
		_pool_sizes.resize(_layout->bindings().size());
		for (size_t i = 0; i < _pool_sizes.size(); ++i)
		{
			const auto& b = _layout->bindings()[i];
			_pool_sizes[i] = VkDescriptorPoolSize{
				.type = b.descriptorType,
				.descriptorCount = b.descriptorCount,
			};
		}

		VkDescriptorPoolCreateInfo vk_ci = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = _flags,
			.maxSets = _max_sets,
			.poolSizeCount = (uint32_t)_pool_sizes.size(),
			.pPoolSizes = _pool_sizes.data(),
		};

		create(vk_ci);
	}

	DescriptorPool::~DescriptorPool()
	{
		if (_handle)
		{
			destroy();
		}
	}

	void DescriptorPool::create(VkDescriptorPoolCreateInfo const& ci)
	{
		VK_CHECK(vkCreateDescriptorPool(_app->device(), &ci, nullptr, &_handle), "Failed to create a descriptor pool.");
	}

	void DescriptorPool::destroy()
	{
		vkDestroyDescriptorPool(_app->device(), _handle, nullptr);
		_handle = VK_NULL_HANDLE;
	}
}