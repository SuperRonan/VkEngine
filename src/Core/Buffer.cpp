#include "Buffer.hpp"
#include <cassert>

namespace vkl
{
	Buffer::Buffer(CreateInfo const& ci) :
		VkObject(ci.app, ci.name),
		_size(ci.size),
		_usage(ci.usage),
		_queues(std::filterRedundantValues(ci.queues)),
		_sharing_mode(_queues.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE),
		_mem_usage(ci.mem_usage),
		_allocator(ci.allocator ? ci.allocator : _app->allocator())
	{
		if (ci.create_on_construct)
		{
			create();
		}
	}

	Buffer::~Buffer()
	{
		if (_buffer != VK_NULL_HANDLE)
		{
			destroyBuffer();
		}
	}

	void Buffer::create()
	{
		assert(_buffer == VK_NULL_HANDLE);
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

		VK_CHECK(vmaCreateBuffer(_allocator, &ci, &alloc, &_buffer, &_alloc, nullptr), "Failed to create a buffer.");

		if (!name().empty())
		{
			VkDebugUtilsObjectNameInfoEXT buffer_name = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext = nullptr,
				.objectType = VK_OBJECT_TYPE_BUFFER,
				.objectHandle = (uint64_t)_buffer,
				.pObjectName = name().c_str(),
			};
			_app->nameObject(buffer_name);
		}
	}

	void Buffer::destroyBuffer()
	{
		assert(_buffer != VK_NULL_HANDLE);
		if (_data)
		{
			unMap();
		}
		vmaDestroyBuffer(_allocator, _buffer, _alloc);
		_buffer = VK_NULL_HANDLE;
		_alloc = nullptr;
		_size = 0;
	}

	void Buffer::map()
	{
		assert(_buffer != VK_NULL_HANDLE);
		vmaMapMemory(_allocator, _alloc, &_data);
	}

	void Buffer::unMap()
	{
		assert(_buffer != VK_NULL_HANDLE);
		vmaUnmapMemory(_allocator, _alloc);
		_data = nullptr;
	}

	//StagingPool::StagingBuffer * Buffer::copyToStaging(void* data, size_t size)
	//{
	//	if (size == 0)	size = _size;
	//	StagingPool::StagingBuffer* sb = _app->stagingPool().getStagingBuffer(size);

	//	std::memcpy(sb->data, data, size);

	//	return sb;
	//}

	//void Buffer::recordCopyStagingToBuffer(VkCommandBuffer cmd, StagingPool::StagingBuffer* sb)
	//{
	//	VkBufferCopy copy{
	//		.srcOffset = 0,
	//		.dstOffset = 0,
	//		.size = std::min(_size, sb->size),
	//	};

	//	vkCmdCopyBuffer(cmd, sb->buffer, _buffer, 1, &copy);
	//}
}