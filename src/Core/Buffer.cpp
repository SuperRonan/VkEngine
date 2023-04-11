#include "Buffer.hpp"
#include <cassert>

namespace vkl
{
	BufferInstance::BufferInstance(CreateInfo const& ci) :
		VkObject(ci.app, ci.name),
		_ci(ci.ci),
		_aci(ci.aci),
		_allocator(ci.allocator)
	{
		create();
	}

	BufferInstance::~BufferInstance()
	{
		if (!!_buffer)
		{
			destroy();
		}
	}

	void BufferInstance::create()
	{
		assert(!_buffer);
		VK_CHECK(vmaCreateBuffer(_allocator, &_ci, &_aci, &_buffer, &_alloc, nullptr), "Failed to create a buffer.");

		setName();
	}

	void BufferInstance::destroy()
	{
		assert(!!_buffer);

		if (!!_data)
		{
			unMap();
		}

		vmaDestroyBuffer(_allocator, _buffer, _alloc);
		_buffer = VK_NULL_HANDLE;
		_alloc = VMA_NULL;
	}

	void BufferInstance::setName()
	{
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


	void BufferInstance::map()
	{
		assert(_buffer != VK_NULL_HANDLE);
		vmaMapMemory(_allocator, _alloc, &_data);
	}

	void BufferInstance::unMap()
	{
		assert(_buffer != VK_NULL_HANDLE);
		vmaUnmapMemory(_allocator, _alloc);
		_data = nullptr;
	}



	Buffer::Buffer(CreateInfo const& ci) :
		AbstractInstanceHolder(ci.app, ci.name),
		_size(ci.size),
		_usage(ci.usage),
		_queues(std::filterRedundantValues(ci.queues)),
		_sharing_mode(_queues.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE),
		_mem_usage(ci.mem_usage),
		_allocator(ci.allocator ? ci.allocator : _app->allocator())
	{
		if (ci.create_on_construct)
		{
			createInstance();
		}
	}

	Buffer::~Buffer()
	{
		
	}

	void Buffer::createInstance()
	{
		BufferInstance::CreateInfo ci{
			.app = application(),
			.name = name(),
			.ci = VkBufferCreateInfo {
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = _size,
				.usage = _usage,
				.sharingMode = _sharing_mode,
				.queueFamilyIndexCount = (uint32_t)_queues.size(),
				.pQueueFamilyIndices = _queues.data(),
			},
			.aci = VmaAllocationCreateInfo{
				.usage = _mem_usage,
			},
			.allocator = _allocator,
		};
		
		_inst = std::make_shared<BufferInstance>(ci);
	}

	void Buffer::destroyInstance()
	{
		if (_inst)
		{
			callInvalidationCallbacks();
			_inst = nullptr;
		}
	}

	bool Buffer::updateResource()
	{
		if (!_inst)
		{
			createInstance();
			return true;
		}

		return false;
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