#include "StagingPool.hpp"
#include <cassert>
#include <iostream>

namespace vkl
{
	StagingPool::StagingPool(CreateInfo const& ci):
		VkObject(ci.app, ci.name),
		_usage(ci.usage),
		_allocator(ci.allocator)
	{}

	StagingPool::~StagingPool()
	{
		clearFreeBuffers();


		//_mutex.lock();
		//if (_used_buffers.size() > 0)
		//{
		//	std::cerr << "Destroying in use staging buffers!" << std::endl; // Outdated
		//	for (std::shared_ptr<BufferInstance>& sb : _used_buffers)
		//	{
		//		if (sb->createInfo().size)
		//		{
		//			sb = nullptr;
		//		}
		//	}
		//}
		//_mutex.unlock();
	}

	std::shared_ptr<BufferInstance> StagingPool::getStagingBuffer(size_t size)
	{
		std::unique_lock lock(_mutex);
		
		// Find the smallest big enough free buffer
		// TODO start from the end if closer to it
		auto it = _free_buffers.begin(), end = _free_buffers.end();
		while (it != end)
		{
			if ((*it)->createInfo().size >= size)
			{
				break;
			}
			++it;
		}

		std::shared_ptr<BufferInstance> res;

		if (it == end) // Did not find any: allocate a new one (Maybe re allocate a too small one if available)
		{
			size_t buffer_size = std::alignUp(size, size_t(1024));
			VkBufferCreateInfo ci{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.size = buffer_size,
				.usage = VK_BUFFER_USAGE_TRANSFER_BITS,
			};
			VmaAllocationCreateInfo aci{
				.usage = _usage,
			};
			res = std::make_shared<BufferInstance>(BufferInstance::CI{
				.app = _app,
				.name = name() + ".StagingBuffer",
				.ci = ci,
				.aci = aci,
				.allocator = application()->allocator(),
			});
		}
		else
		{
			res = *it;
			_free_buffers.erase(it);
		}
		return res;
	}

	void StagingPool::releaseStagingBuffer(std::shared_ptr<BufferInstance> staging_buffer)
	{
		std::unique_lock lock(_mutex);
		auto it = _free_buffers.begin(), end = _free_buffers.end();
		// TODO faster insertion
		while (it != end)
		{
			if (staging_buffer->createInfo().size <= (*it)->createInfo().size)
			{
				break;
			}
			++it;
		}

		_free_buffers.insert(it, staging_buffer);
	}

	void StagingPool::clearFreeBuffers()
	{
		_mutex.lock();
		{	
			for (auto& b : _free_buffers)
			{
				
			}
			_free_buffers.clear();
		}
		_mutex.unlock();
	}
}