#include <vkl/VkObjects/DescriptorSet.hpp>
#include <cassert>

namespace vkl
{

	DescriptorSet::DescriptorSet(CreateInfo const& ci) :
		VkObject(ci.app, ci.name),
		_layout(ci.layout),
		_pool(ci.pool)
	{
		VkDescriptorSetLayout l = *_layout;
		VkDescriptorSetAllocateInfo alloc{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = *_pool,
			.descriptorSetCount = 1,
			.pSetLayouts = &l,
		};
		allocate(alloc);
		setVkName();
	}

	DescriptorSet::~DescriptorSet()
	{
		if (_handle)
		{
			destroy();
		}
	}

	void DescriptorSet::allocate(VkDescriptorSetAllocateInfo const& alloc)
	{
		assert(!_handle);
		VK_CHECK(vkAllocateDescriptorSets(_app->device(), &alloc, &_handle), "Failed to allocate a descriptor set.");
	}

	void DescriptorSet::destroy()
	{
		assert(_handle);
		if (_pool->flags() & VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
		{
			vkFreeDescriptorSets(_app->device(), *_pool, 1, &_handle);
		}
		_handle = VK_NULL_HANDLE;
	}

	void DescriptorSet::setVkName()
	{
		application()->nameVkObjectIFP(VkDebugUtilsObjectNameInfoEXT{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.pNext = nullptr,
			.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET,
			.objectHandle = reinterpret_cast<uint64_t>(_handle),
			.pObjectName = name().c_str(),
		});
	}
	
} // namespace vkl
