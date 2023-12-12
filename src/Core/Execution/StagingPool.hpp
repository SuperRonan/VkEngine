#pragma once

#include "vk_mem_alloc.h"
#include <mutex>
#include <vector>
#include <Core/VkObjects/Buffer.hpp>
#include <Core/Execution/ResourceState.hpp>

namespace vkl
{
	class StagingPool : public VkObject
	{
	public:
		
	protected:

		VmaMemoryUsage _usage;

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
			VmaMemoryUsage usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
		};
		using CI = CreateInfo;

		StagingPool(CreateInfo const& ci);

		virtual ~StagingPool() override;

		std::shared_ptr<BufferInstance> getStagingBuffer(size_t size);

		void releaseStagingBuffer(std::shared_ptr<BufferInstance> staging_buffer);

		void clearFreeBuffers();

	};

	class StagingBuffer : public VkObject
	{
	protected:

		StagingPool* _pool;
		std::shared_ptr<BufferInstance> _buffer;

	public:

		StagingBuffer(StagingPool* pool, size_t size) :
			VkObject(pool->application(), ""s),
			_pool(pool)
		{
			assert(size);
			_buffer = _pool->getStagingBuffer(size);
		}

		StagingBuffer(StagingPool* pool, std::shared_ptr<BufferInstance> buffer) :
			VkObject(pool->application(), ""s),
			_pool(pool),
			_buffer(buffer)
		{}

		virtual ~StagingBuffer() override
		{
			_pool->releaseStagingBuffer(_buffer);
		}

		std::shared_ptr<BufferInstance> const& buffer()const
		{
			return _buffer;
		}
	};
}