#include "CommandBuffer.hpp"
#include <cassert>

namespace vkl
{
	CommandBuffer::CommandBuffer(VkApplication* app, VkCommandBuffer handle, std::shared_ptr<CommandPool> pool, VkCommandBufferLevel level):
		VkObject(app),
		_pool(std::move(pool)),
		_level(level),
		_handle(handle)
	{}


	CommandBuffer::CommandBuffer(CommandBuffer&& other):
		VkObject(std::move(other)),
		_pool(other._pool),
		_handle(other._handle)
	{
		other._pool = VK_NULL_HANDLE;
		other._handle = VK_NULL_HANDLE;
	}

	CommandBuffer& CommandBuffer::operator=(CommandBuffer&& other)
	{
		VkObject::operator=(std::move(other));
		std::swap(_pool, other._pool);
		std::swap(_handle, other._handle);
		return *this;
	}

	CommandBuffer::CommandBuffer(std::shared_ptr<CommandPool> pool, VkCommandBufferLevel level) :
		VkObject(*pool),
		_pool(std::move(pool)),
		_level(level)
	{
		VkCommandBufferAllocateInfo alloc{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = *_pool,
			.level = _level,
			.commandBufferCount = 1,
		};
		allocate(alloc);
	}
	
	CommandBuffer::~CommandBuffer()
	{
		if (_handle)
		{
			destroy();
		}
	}

	void CommandBuffer::destroy()
	{
		assert(_handle);
		vkFreeCommandBuffers(_app->device(), *_pool, 1, &_handle);
		_handle = VK_NULL_HANDLE;
	}

	void CommandBuffer::allocate(VkCommandBufferAllocateInfo const& alloc)
	{
		assert(!_handle);

		assert(alloc.level == _level);
		VK_CHECK(vkAllocateCommandBuffers(_app->device(), &alloc, &_handle), "Failed to allocate a Command Buffer.");
	}

	void CommandBuffer::begin(VkCommandBufferUsageFlags usage)
	{
		assert(_handle);
		VkCommandBufferBeginInfo bi{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = usage,
			.pInheritanceInfo = nullptr,
		};
		vkBeginCommandBuffer(_handle, &bi);
	}

	void CommandBuffer::end()
	{
		vkEndCommandBuffer(_handle);
	}

	void CommandBuffer::reset(VkCommandBufferResetFlags flags)
	{
		vkResetCommandBuffer(_handle, flags);
	}

	void CommandBuffer::submit(VkQueue queue)
	{
		VkSubmitInfo submission{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &_handle,
		};
		vkQueueSubmit(queue, 1, &submission, VK_NULL_HANDLE);
	}

	void CommandBuffer::submitAndWait(VkQueue queue)
	{
		submit(queue);
		vkQueueWaitIdle(queue);
	}
}