#pragma once

#include <Core/App/VkApplication.hpp>
#include <set>
#include <Core/Utils/stl_extension.hpp>
#include "AbstractInstance.hpp"
#include <Core/Execution/UpdateContext.hpp>

#ifndef VMA_NULL
#define VMA_NULL nullptr
#endif

namespace vkl
{
	class BufferInstance : public VkObject
	{
	protected:

		VkBufferCreateInfo _ci;
		VmaAllocationCreateInfo  _aci;
		VkBuffer _buffer = VK_NULL_HANDLE;
		VmaAllocator _allocator = VMA_NULL;
		VmaAllocation _alloc = VMA_NULL;

		void* _data = nullptr;

		void create();

		void setName();

		void destroy();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VkBufferCreateInfo ci;
			VmaAllocationCreateInfo aci;
			VmaAllocator allocator = VMA_NULL;
		};
		using CI = CreateInfo;

		BufferInstance(CreateInfo const& ci);

		BufferInstance(BufferInstance const&) = delete;
		BufferInstance(BufferInstance &&) = delete;

		BufferInstance& operator=(BufferInstance const&) = delete;
		BufferInstance& operator=(BufferInstance &&) = delete;

		virtual ~BufferInstance() override;

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

		constexpr const VkBufferCreateInfo & createInfo()const
		{
			return _ci;
		}

		constexpr const VmaAllocationCreateInfo& allocationCreateInfo()const
		{
			return _aci;
		}

		constexpr VmaAllocation allocation()const
		{
			return _alloc;
		}

		constexpr VmaAllocator allocator()const
		{
			return _allocator;
		}

	};

	class Buffer : public InstanceHolder<BufferInstance>
	{
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = "";
			DynamicValue<VkDeviceSize> size = VkDeviceSize(0);
			VkBufferUsageFlags usage = 0;
			std::vector<uint32_t> queues = {};
			VmaMemoryUsage mem_usage = VMA_MEMORY_USAGE_MAX_ENUM;

			VmaAllocator allocator = nullptr;
			bool create_on_construct = false;
		};
		using CI = CreateInfo;

	protected:

		DynamicValue<VkDeviceSize> _size = 0;
		VkBufferUsageFlags _usage = 0;
		std::vector<uint32_t> _queues = {};
		VkSharingMode _sharing_mode = VK_SHARING_MODE_MAX_ENUM;
		VmaMemoryUsage _mem_usage = VMA_MEMORY_USAGE_MAX_ENUM;
		VmaAllocator _allocator = nullptr;

		

		void destroyInstance();

	public:

		Buffer(CreateInfo const& ci);

		Buffer(VkApplication * app=nullptr) noexcept:
			InstanceHolder<BufferInstance>(app, ""sv)
		{}

		virtual ~Buffer() override;

		constexpr const DynamicValue<VkDeviceSize> & size()const
		{
			return _size;
		}

		constexpr auto usage()const
		{
			return _usage;
		}

		constexpr auto allocator()const
		{
			return _allocator;
		}

		void createInstance();

		bool updateResource(UpdateContext & ctx);

		//StagingPool::StagingBuffer * copyToStaging(void* data, size_t size = 0);

		//void recordCopyStagingToBuffer(VkCommandBuffer cmd, StagingPool::StagingBuffer * sb);
	};
}