#pragma once

#include "VkApplication.hpp"
#include <set>
#include <Utils/stl_extension.hpp>

namespace vkl
{
	class Buffer : public VkObject
	{
	public:

		struct CreateInfo
		{
			std::string name = "";
			VkDeviceSize size = 0;
			VkBufferUsageFlags usage = 0;
			std::vector<uint32_t> queues = {};
			VmaMemoryUsage mem_usage = VMA_MEMORY_USAGE_MAX_ENUM;
			VmaAllocator allocator = nullptr;
			bool create_on_construct = false;
		};
		using CI = CreateInfo;

	protected:

		size_t _size = 0;
		VkBufferUsageFlags _usage = 0;
		std::vector<uint32_t> _queues = {};
		VkSharingMode _sharing_mode = VK_SHARING_MODE_MAX_ENUM;
		VmaMemoryUsage _mem_usage = VMA_MEMORY_USAGE_MAX_ENUM;
		VkBuffer _buffer = VK_NULL_HANDLE;
		VmaAllocator _allocator = nullptr;
		VmaAllocation _alloc = nullptr;

		void* _data = nullptr;

	public:

		Buffer(VkApplication* app, CreateInfo const& ci);

		void create();

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
			_allocator(other._allocator),
			_alloc(other._alloc),
			_data(other._data)
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
			std::swap(_allocator, other._allocator);
			std::swap(_alloc, other._alloc);
			std::swap(_data, other._data);
			return *this;
		}

		~Buffer();

		
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

		constexpr auto allocator()const
		{
			return _allocator;
		}

		void map();

		void unMap();

		constexpr void* data()
		{
			return _data;
		}

		constexpr const void* data() const
		{
			return _data;
		}

		//StagingPool::StagingBuffer * copyToStaging(void* data, size_t size = 0);

		//void recordCopyStagingToBuffer(VkCommandBuffer cmd, StagingPool::StagingBuffer * sb);
	};
}