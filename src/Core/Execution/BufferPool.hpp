#pragma once

#include "vk_mem_alloc.h"
#include <mutex>
#include <vector>
#include <Core/VkObjects/Buffer.hpp>
#include <Core/Execution/ResourceState.hpp>

#include <Core/Execution/BufferPool.hpp>

namespace vkl
{
	// TODO: Maybe allocate BufferAndRange rather than a single buffer
	//
	class BufferPool : public VkObject
	{
	public:
		
	protected:

		size_t _minimum_size = 1024;
		VkDeviceSize _min_align = 1;
		VkBufferUsageFlags _usage;
		VmaMemoryUsage _mem_usage;

		VmaAllocator _allocator = nullptr;

		// Sorted from smallest to largest
		std::deque<std::shared_ptr<BufferInstance>> _free_buffers;
		//// Not Sorted
		//std::deque<std::shared_ptr<BufferInstance>> _used_buffers;
		
		std::mutex _mutex;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VmaAllocator allocator = nullptr;
			VkBufferUsageFlags usage = 0;
			VmaMemoryUsage mem_usage = VmaMemoryUsage::VMA_MEMORY_USAGE_UNKNOWN;
			size_t min_size = 1024;
			VkDeviceSize min_align = 1;
		};
		using CI = CreateInfo;

		BufferPool(CreateInfo const& ci);

		virtual ~BufferPool() override;

		std::shared_ptr<BufferInstance> get(size_t size);

		void release(std::shared_ptr<BufferInstance> const& b);

		void clearFreeBuffers();

	};

	class PooledBuffer : public VkObject
	{
	protected:

		BufferPool* _pool;
		std::shared_ptr<BufferInstance> _buffer;

	public:

		PooledBuffer(BufferPool* pool, size_t size) :
			VkObject(pool->application(), ""s),
			_pool(pool)
		{
			assert(size);
			_buffer = _pool->get(size);
		}

		PooledBuffer(BufferPool* pool, std::shared_ptr<BufferInstance> buffer) :
			VkObject(pool->application(), ""s),
			_pool(pool),
			_buffer(buffer)
		{}

		virtual ~PooledBuffer() override
		{
			_pool->release(_buffer);
		}

		std::shared_ptr<BufferInstance> const& buffer()const
		{
			return _buffer;
		}

		BufferSegmentInstance bufferSegment()const
		{
			return BufferSegmentInstance{
				.buffer = _buffer,
				.range = _buffer->fullRange(),
			};
		}
	};
}