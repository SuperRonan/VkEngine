#pragma once

#include "VkApplication.hpp"

namespace vkl
{
	class Buffer : public VkObject
	{
	protected:

		size_t _size = 0;
		VkBufferUsageFlags _usage;
		VkSharingMode _sharing_mode;
		std::vector<uint32_t> _queues;
		VmaMemoryUsage _mem_usage;
		VkBuffer _buffer = VK_NULL_HANDLE;
		VmaAllocation _alloc = nullptr;

	public:



		constexpr Buffer(VkApplication * app=nullptr) noexcept:
			VkObject(app)
		{}

		constexpr Buffer(Buffer const&) noexcept = delete;

		constexpr Buffer(Buffer&& other) noexcept :
			_size(other._size),
			_usage(other._usage),
			_sharing_mode(other._sharing_mode),
			_queues(other._queues),
			_mem_usage(other._mem_usage),
			_buffer(other._buffer),
			_alloc(other._alloc)
		{
			other._buffer = VK_NULL_HANDLE;
			other._alloc = nullptr;
		}

		constexpr Buffer& operator=(Buffer const&) noexcept = delete;

		constexpr Buffer& operator=(Buffer&& other) noexcept
		{
			VkObject::operator=(std::move(other));
			std::swap(_size, other._size);
			std::swap(_usage, other._usage);
			std::swap(_sharing_mode, other._sharing_mode);
			std::swap(_queues, other._queues);
			std::swap(_mem_usage, other._mem_usage);
			std::swap(_buffer, other._buffer);
			std::swap(_alloc, other._alloc);
			return *this;
		}

		~Buffer();

		void createBuffer(size_t size, VkBufferUsageFlags usage, VmaMemoryUsage mem_usage=VMA_MEMORY_USAGE_GPU_ONLY, std::vector<uint32_t> const& queues = {});

		void destroyBuffer();

		constexpr VkBuffer buffer()const
		{
			return _buffer;
		}

		constexpr auto handle()const
		{
			return buffer();
		}

		constexpr operator VkBuffer()const
		{
			return buffer();
		}

		constexpr size_t size()const
		{
			return _size;
		}

		constexpr auto usage()const
		{
			return _usage;
		}

		constexpr auto alloc()const
		{
			return _alloc;
		}

		StagingPool::StagingBuffer * copyToStaging(void* data, size_t size = 0);

		void recordCopyStagingToBuffer(VkCommandBuffer cmd, StagingPool::StagingBuffer * sb);
	};
}