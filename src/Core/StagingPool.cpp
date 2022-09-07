#include "StagingPool.hpp"
#include <cassert>
#include <iostream>

namespace vkl
{
	StagingPool::StagingPool(VkApplication * app, VmaAllocator allocator):
		VkObject(app),
		_allocator(allocator)
	{}

	StagingPool::~StagingPool()
	{
		_mutex.lock();
		for(StagingBuffer & sb : _free_buffers)
		{
			if (sb->size())
			{
				sb = nullptr;
			}
		}

		if (_used_buffers.size() > 0)
		{
			std::cerr << "Destroying in use staging buffers!" << std::endl;
			for (StagingBuffer& sb : _used_buffers)
			{
				if (sb->size())
				{
					sb = nullptr;
				}
			}
		}
		_mutex.unlock();
	}

	StagingPool::StagingBuffer  StagingPool::getStagingBuffer(size_t size)
	{
		_mutex.lock();
		
		// Find the smallest big enough free buffer
		StagingBuffer res = nullptr;
		auto it = _free_buffers.begin(), end = _free_buffers.end(), found_it = end;
		while (it != end)
		{
			if ((*it)->size() >= size)
			{
				if (res)
				{
					if (res->size ()> (*it)->size())
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
			res = std::make_shared<Buffer>(Buffer::CI{
				.app = _app,
				.size = size,
				.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				.mem_usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
				.allocator = _allocator,
				.create_on_construct = true,
			});

			_free_buffers.push_back(res);
			found_it = _free_buffers.end() - 1;
		}
		
		_free_buffers.erase(found_it);

		_used_buffers.push_back(res);

		_mutex.unlock();
		return res;
	}

	void StagingPool::releaseStagingBuffer(StagingBuffer staging_buffer)
	{
		_mutex.lock();

		auto it = std::find_if(_used_buffers.begin(), _used_buffers.end(), [&staging_buffer](StagingBuffer const& other)
			{
				return staging_buffer.get() == other.get();
			});
		assert(it != _used_buffers.end());
		_used_buffers.erase(it);
		_free_buffers.push_back(staging_buffer);

		_mutex.unlock();
	}

	void StagingPool::setAllocator(VmaAllocator allocator)
	{
		assert(_allocator == nullptr);
		_allocator = allocator;
	}
}