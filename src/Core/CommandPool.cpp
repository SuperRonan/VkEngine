
#include "CommandPool.hpp"
#include <cassert>

namespace vkl
{
    CommandPool::~CommandPool()
    {
        if (_handle)
        {
            destroy();
        }
    }

    void CommandPool::create(VkCommandPoolCreateInfo const& ci)
    {
        assert(!_handle);
        VK_CHECK(vkCreateCommandPool(_app->device(), &ci, nullptr, &_handle), "Failed to create a command pool");
    }

    void CommandPool::destroy()
    {
        assert(_handle);
        vkDestroyCommandPool(_app->device(), _handle, nullptr);
    }

    CommandPool::CommandPool(VkApplication* app, uint32_t index, VkCommandPoolCreateFlags flags) :
        VkObject(app),
        _queue_family_index(index),
        _flags(flags)
    {
        VkCommandPoolCreateInfo ci{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = _flags,
            .queueFamilyIndex = _queue_family_index,
        };
        create(ci);
    }
}