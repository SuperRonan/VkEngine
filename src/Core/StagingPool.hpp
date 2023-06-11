#pragma once

#include "vk_mem_alloc.h"
#include <mutex>
#include <vector>
#include "Buffer.hpp"

namespace vkl
{
	class Executor;

	class StagingPool : public VkObject
	{
	public:
		
	protected:

		VmaAllocator _allocator = nullptr;

		std::vector<std::shared_ptr<Buffer>> _free_buffers, _used_buffers;
		
		std::mutex _mutex;

		Executor* _exec;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VmaAllocator allocator = nullptr;
			Executor* exec = nullptr;
		};
		using CI = CreateInfo;

		StagingPool(CreateInfo const& ci);

		virtual ~StagingPool() override;

		std::shared_ptr<Buffer> getStagingBuffer(size_t size);

		void releaseStagingBuffer(std::shared_ptr<Buffer> staging_buffer);

		void clearFreeBuffers();

	};

	class StagingBuffer : public VkObject
	{
	protected:

		StagingPool* _pool;
		std::shared_ptr<Buffer> _buffer;

	public:

		StagingBuffer(StagingPool* pool, size_t size) :
			VkObject(pool->application(), ""s),
			_pool(pool)
		{
			assert(size);
			_buffer = _pool->getStagingBuffer(size);
		}

		StagingBuffer(StagingPool* pool, std::shared_ptr<Buffer> buffer) :
			VkObject(pool->application(), ""s),
			_pool(pool),
			_buffer(buffer)
		{}

		virtual ~StagingBuffer() override
		{
			_pool->releaseStagingBuffer(_buffer);
		}

		std::shared_ptr<Buffer> const& buffer()const
		{
			return _buffer;
		}
	};
}