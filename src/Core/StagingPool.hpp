#pragma once

#include "vk_mem_alloc.h"
#include <mutex>
#include <vector>

namespace vkl
{
	class StagingPool
	{
	public:
		
		struct StagingBuffer
		{
			VkBuffer buffer = VK_NULL_HANDLE;
			VmaAllocation allocation = nullptr;
			void* data = nullptr;
			size_t size = 0;
		};

	protected:

		VmaAllocator _allocator = nullptr;

		std::vector<StagingBuffer*> _free_buffers, _used_buffers;
		
		std::mutex _mutex;

		void allocStagingBuffer(StagingBuffer & staging_buffer);

		void freeStagingBuffer(StagingBuffer & staging_buffer);

	public:

		StagingPool(VmaAllocator alloc=nullptr);

		StagingPool(StagingPool const&) = delete;
		StagingPool(StagingPool&&) = delete;

		StagingPool& operator=(StagingPool const&) = delete;
		StagingPool& operator=(StagingPool&&) = delete;

		~StagingPool();

		void setAllocator(VmaAllocator alloc);

		StagingBuffer * getStagingBuffer(size_t size);

		void releaseStagingBuffer(StagingBuffer * staging_buffer);

	};
}