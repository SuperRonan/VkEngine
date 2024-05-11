
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

    void CommandPool::setVkNameIFP()
    {
        application()->nameVkObjectIFP(VK_OBJECT_TYPE_COMMAND_POOL, reinterpret_cast<uint64_t>(_handle), name());
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

    CommandPool::CommandPool(CreateInfo const& ci) :
        VkObject(ci.app, ci.name),
        _queue_family(ci.queue_family),
        _flags(ci.flags)
    {
        VkCommandPoolCreateInfo vk_ci{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = _flags,
            .queueFamilyIndex = _queue_family,
        };
        create(vk_ci);
        setVkNameIFP();
    }
}