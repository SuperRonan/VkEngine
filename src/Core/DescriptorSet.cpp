#include "DescriptorSet.hpp"
#include <cassert>

namespace vkl
{

    DescriptorSet::DescriptorSet(std::shared_ptr<DescriptorSetLayout> layout, std::shared_ptr<DescriptorPool> pool) :
        VkObject(*layout),
        _layout(std::move(layout)),
        _pool(std::move(pool))
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
    }

    DescriptorSet::DescriptorSet(std::shared_ptr<DescriptorSetLayout> layout, std::shared_ptr<DescriptorPool> pool, VkDescriptorSet handle) :
        VkObject(*layout),
        _layout(std::move(layout)),
        _pool(std::move(pool)),
        _handle(handle)
    {}

    DescriptorSet::DescriptorSet(DescriptorSet&& other) noexcept :
        VkObject(std::move(other)),
        _layout(std::move(other._layout)),
        _pool(std::move(other._pool)),
        _handle(other._handle)
    {
        other._handle = VK_NULL_HANDLE;
    }

    DescriptorSet& DescriptorSet::operator=(DescriptorSet&& other) noexcept
    {
        VkObject::operator=(std::move(other));
        std::swap(_layout, other._layout);
        std::swap(_pool, other._pool);
        std::swap(_handle, other._handle);
        return *this;
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
    
} // namespace vkl
