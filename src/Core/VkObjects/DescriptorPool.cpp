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
		create();
		setVkName();
	}

	DescriptorPool::DescriptorPool(CreateInfoRaw const& ci) :
		VkObject(ci.app, ci.name),
		_pool_sizes(ci.sizes),
		_max_sets(ci.max_sets),
		_flags(ci.flags)
	{
		create();
		setVkName();
	}

	DescriptorPool::~DescriptorPool()
	{
		if (_handle)
		{
			destroy();
		}
	}

	void DescriptorPool::create()
	{
		VkDescriptorPoolCreateInfo ci = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = _flags,
			.maxSets = _max_sets,
			.poolSizeCount = (uint32_t)_pool_sizes.size(),
			.pPoolSizes = _pool_sizes.data(),
		};
		VK_CHECK(vkCreateDescriptorPool(_app->device(), &ci, nullptr, &_handle), "Failed to create a descriptor pool.");
	}

	void DescriptorPool::destroy()
	{
		vkDestroyDescriptorPool(_app->device(), _handle, nullptr);
		_handle = VK_NULL_HANDLE;
	}

	void DescriptorPool::setVkName()
	{
		application()->nameObject(VkDebugUtilsObjectNameInfoEXT{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.pNext = nullptr,
			.objectType = VK_OBJECT_TYPE_DESCRIPTOR_POOL,
			.objectHandle = reinterpret_cast<uint64_t>(_handle),
			.pObjectName = name().c_str(),
		});
	}
}