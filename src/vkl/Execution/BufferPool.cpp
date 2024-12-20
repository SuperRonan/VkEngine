#include <vkl/Execution/BufferPool.hpp>
#include <cassert>
#include <iostream>

namespace vkl
{
	BufferPool::BufferPool(CreateInfo const& ci):
		VkObject(ci.app, ci.name),
		_min_align(ci.min_align),
		_minimum_size(ci.min_size),
		_usage(ci.usage),
		_mem_usage(ci.mem_usage),
		_allocator(ci.allocator)
	{}

	BufferPool::~BufferPool()
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

	std::shared_ptr<BufferInstance> BufferPool::get(size_t size)
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
			size_t buffer_size = std::alignUp(size, _minimum_size);
			VkBufferCreateInfo ci{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.size = buffer_size,
				.usage = _usage,
			};
			VmaAllocationCreateInfo aci{
				.usage = _mem_usage,
			};
			res = std::make_shared<BufferInstance>(BufferInstance::CI{
				.app = _app,
				.name = name() + ".PooledBuffer",
				.ci = ci,
				.aci = aci,
				.min_align = _min_align,
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

	void BufferPool::release(std::shared_ptr<BufferInstance> const& b)
	{
		std::unique_lock lock(_mutex);
		auto it = _free_buffers.begin(), end = _free_buffers.end();
		// TODO faster insertion (sorted)
		while (it != end)
		{
			if (b->createInfo().size <= (*it)->createInfo().size)
			{
				break;
			}
			++it;
		}

		_free_buffers.insert(it, b);
	}

	void BufferPool::clearFreeBuffers()
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