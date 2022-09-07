#pragma once

#include "vk_mem_alloc.h"
#include <mutex>
#include <vector>
#include "Buffer.hpp"

namespace vkl
{
	class StagingPool : public VkObject
	{
	public:
		
		using StagingBuffer = std::shared_ptr<Buffer>;

	protected:

		VmaAllocator _allocator = nullptr;

		std::vector<StagingBuffer> _free_buffers, _used_buffers;
		
		std::mutex _mutex;

		void allocStagingBuffer(StagingBuffer & staging_buffer);

	public:

		StagingPool(VkApplication * app, VmaAllocator alloc=nullptr);

		StagingPool(StagingPool const&) = delete;
		StagingPool(StagingPool&&) = delete;

		StagingPool& operator=(StagingPool const&) = delete;
		StagingPool& operator=(StagingPool&&) = delete;

		~StagingPool();

		void setAllocator(VmaAllocator alloc);

		StagingBuffer getStagingBuffer(size_t size);

		void releaseStagingBuffer(StagingBuffer staging_buffer);

	};
}