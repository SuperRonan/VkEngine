#include "Buffer.hpp"
#include <cassert>

namespace vkl
{
	Buffer::~Buffer()
	{
		if (_buffer != VK_NULL_HANDLE)
		{
			destroyBuffer();
		}
	}

	void Buffer::createBuffer(size_t size, VkBufferUsageFlags usage, VmaMemoryUsage mem_usage, std::vector<uint32_t> const& queues)
	{
		assert(_buffer == VK_NULL_HANDLE);

		_size = size;
		_usage = usage;
		_sharing_mode = queues.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
		_queues = queues;
		_mem_usage = mem_usage;
		VkBufferCreateInfo ci{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = _size,
			.usage = _usage,
			.sharingMode = _sharing_mode,
			.queueFamilyIndexCount = (uint32_t)_queues.size(),
			.pQueueFamilyIndices = _queues.data(),
		};

		VmaAllocationCreateInfo alloc{
			.usage = _mem_usage,
		};

		VK_CHECK(vmaCreateBuffer(_app->allocator(), &ci, &alloc, &_buffer, &_alloc, nullptr), "Failed to create a buffer.");
	}

	void Buffer::destroyBuffer()
	{
		assert(_buffer != VK_NULL_HANDLE);

		vmaDestroyBuffer(_app->allocator(), _buffer, _alloc);
		_buffer = VK_NULL_HANDLE;
		_alloc = nullptr;
	}

	StagingPool::StagingBuffer * Buffer::copyToStaging(void* data, size_t size)
	{
		if (size == 0)	size = _size;
		StagingPool::StagingBuffer* sb = _app->stagingPool().getStagingBuffer(size);

		std::memcpy(sb->data, data, size);

		return sb;
	}

	void Buffer::recordCopyStagingToBuffer(VkCommandBuffer cmd, StagingPool::StagingBuffer* sb)
	{
		VkBufferCopy copy{
			.srcOffset = 0,
			.dstOffset = 0,
			.size = _size,
		};

		vkCmdCopyBuffer(cmd, sb->buffer, _buffer, 1, &copy);
	}
}