#include <vkl/VkObjects/CommandBuffer.hpp>
#include <cassert>

namespace vkl
{
	CommandBuffer::CommandBuffer(VkApplication* app, VkCommandBuffer handle, std::shared_ptr<CommandPool> pool, VkCommandBufferLevel level):
		VkObject(app),
		_pool(std::move(pool)),
		_level(level),
		_handle(handle)
	{}

	CommandBuffer::CommandBuffer(std::shared_ptr<CommandPool> pool, VkCommandBufferLevel level) :
		VkObject(pool->application()),
		_pool(std::move(pool)),
		_level(level)
	{
		allocate();
	}

	CommandBuffer::CommandBuffer(CreateInfo const& ci):
		VkObject(!ci.app ? ci.pool->application() : ci.app, ci.name),
		_pool(ci.pool),
		_level(ci.level)
	{
		if (ci.allocate_on_construct)
		{
			allocate();
		}
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

	void CommandBuffer::allocate()
	{
		assert(!_handle);
		
		VkCommandBufferAllocateInfo alloc{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = *_pool,
			.level = _level,
			.commandBufferCount = 1,
		};

		assert(alloc.level == _level);
		VK_CHECK(vkAllocateCommandBuffers(_app->device(), &alloc, &_handle), "Failed to allocate a Command Buffer.");
		
		if (!name().empty())
		{
			VkDebugUtilsObjectNameInfoEXT cb_name{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext = nullptr,
				.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER,
				.objectHandle = (uint64_t)_handle,
				.pObjectName = name().c_str(),
			};
			_app->nameVkObjectIFP(cb_name);
		}
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