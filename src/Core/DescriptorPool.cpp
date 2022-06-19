#include "DescriptorPool.hpp"

namespace vkl
{
	DescriptorPool::DescriptorPool(DescriptorPool&& other) noexcept :
		VkObject(std::move(other)),
		_layout(std::move(other._layout)),
		_pool_sizes(std::move(other._pool_sizes)),
		_max_sets(other._max_sets),
		_handle(std::move(other._handle))
	{
		other._handle = VK_NULL_HANDLE;
	}

	DescriptorPool& DescriptorPool::operator=(DescriptorPool&& other) noexcept
	{
		VkObject::operator=(std::move(other));
		std::swap(_layout, other._layout);
		std::swap(_pool_sizes, other._pool_sizes);
		std::swap(_max_sets, other._max_sets);
		std::swap(_handle, other._handle);
		return *this;
	}

	DescriptorPool::DescriptorPool(std::shared_ptr<DescriptorSetLayout> layout, uint32_t max_sets):
		VkObject(layout->application()),
		_layout(std::move(layout)),
		_max_sets(max_sets)
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

		VkDescriptorPoolCreateInfo ci = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.maxSets = _max_sets,
			.poolSizeCount = (uint32_t)_pool_sizes.size(),
			.pPoolSizes = _pool_sizes.data(),
		};

		create(ci);
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