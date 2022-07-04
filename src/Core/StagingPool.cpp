#include "StagingPool.hpp"
#include <cassert>
#include <iostream>

namespace vkl
{
	StagingPool::StagingPool(VmaAllocator allocator):
		_allocator(allocator)
	{}

	StagingPool::~StagingPool()
	{
		_mutex.lock();
		for(StagingBuffer * sb : _free_buffers)
		{
			if (sb->size)
			{
				freeStagingBuffer(*sb);
				delete sb;
				sb = nullptr;
			}
		}

		if (_used_buffers.size() > 0)
		{
			std::cerr << "Destroying in use staging buffers!" << std::endl;
			for (StagingBuffer* sb : _used_buffers)
			{
				if (sb->size)
				{
					freeStagingBuffer(*sb);
				}
			}
		}
		_mutex.unlock();
	}

	StagingPool::StagingBuffer * StagingPool::getStagingBuffer(size_t size)
	{
		_mutex.lock();
		
		// Find the smallest big enough free buffer
		StagingBuffer* res = nullptr;
		auto it = _free_buffers.begin(), end = _free_buffers.end(), found_it = end;
		while (it != end)
		{
			if ((*it)->size >= size)
			{
				if (res)
				{
					if (res->size > (*it)->size)
					{
						found_it = it;
						res = *it;
					}
				}
				else
				{
					found_it = it;
					res = *it;
				}
			}
			++it;
		}

		if (res == nullptr) // Did not find any: allocate a new one (Maybe re allocate a too small one if available)
		{
			_free_buffers.push_back(new StagingBuffer());
			_free_buffers.back()->size = size;
			allocStagingBuffer(*_free_buffers.back());
			found_it = _free_buffers.end() - 1;
			res = *found_it;
		}
		
		_free_buffers.erase(found_it);

		_used_buffers.push_back(res);

		_mutex.unlock();
		return res;
	}

	void StagingPool::releaseStagingBuffer(StagingBuffer* staging_buffer)
	{
		_mutex.lock();

		auto it = std::find(_used_buffers.begin(), _used_buffers.end(), staging_buffer);
		assert(it != _used_buffers.end());
		_used_buffers.erase(it);
		_free_buffers.push_back(staging_buffer);

		_mutex.unlock();
	}

	void StagingPool::allocStagingBuffer(StagingBuffer& staging_buffer)
	{
		assert(staging_buffer.buffer == VK_NULL_HANDLE);

		VkBufferCreateInfo buffer_ci{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = staging_buffer.size,
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		};

		VmaAllocationCreateInfo alloc{
			.usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
		};

		vmaCreateBuffer(_allocator, &buffer_ci, &alloc, &staging_buffer.buffer, &staging_buffer.allocation, nullptr);
		vmaMapMemory(_allocator, staging_buffer.allocation, &staging_buffer.data);
	}

	void StagingPool::freeStagingBuffer(StagingBuffer& staging_buffer)
	{
		assert(staging_buffer.size > 0);

		vmaUnmapMemory(_allocator, staging_buffer.allocation);

		vmaDestroyBuffer(_allocator, staging_buffer.buffer, staging_buffer.allocation);

		staging_buffer = StagingBuffer();
	}

	void StagingPool::setAllocator(VmaAllocator allocator)
	{
		assert(_allocator == nullptr);
		_allocator = allocator;
	}
}