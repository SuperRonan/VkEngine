#pragma once

#include "VkApplication.hpp"
#include <set>
#include <Utils/stl_extension.hpp>
#include <Core/AbstractInstance.hpp>


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

		void destroy();

		void setName();

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

	class Buffer : public VkObjectWithCallbacks
	{
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
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
		VmaAllocator _allocator = nullptr;

		std::shared_ptr<BufferInstance> _inst = nullptr;
		
		void createInstance();

		void destroyInstance();

	public:

		Buffer(CreateInfo const& ci);


		constexpr Buffer(VkApplication * app=nullptr) noexcept:
			VkObjectWithCallbacks(app, ""sv)
		{}

		constexpr Buffer(Buffer const&) noexcept = delete;

		Buffer(Buffer&& other) noexcept :
			VkObjectWithCallbacks(std::move(other)),
			_size(other._size),
			_usage(other._usage),
			_sharing_mode(other._sharing_mode),
			_queues(other._queues),
			_mem_usage(other._mem_usage),
			_allocator(other._allocator),
			_inst(std::move(other._inst))
		{

		}

		constexpr Buffer& operator=(Buffer const&) noexcept = delete;

		constexpr Buffer& operator=(Buffer&& other) noexcept
		{
			VkObjectWithCallbacks::operator=(std::move(other));
			std::swap(_size, other._size);
			std::swap(_usage, other._usage);
			std::swap(_sharing_mode, other._sharing_mode);
			std::swap(_queues, other._queues);
			std::swap(_mem_usage, other._mem_usage);
			std::swap(_allocator, other._allocator);
			std::swap(_inst, other._inst);
			return *this;
		}

		~Buffer();

		constexpr const std::shared_ptr<BufferInstance> & instance()const
		{
			return _inst;
		}

		constexpr size_t size()const
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

		bool updateResource();

		//StagingPool::StagingBuffer * copyToStaging(void* data, size_t size = 0);

		//void recordCopyStagingToBuffer(VkCommandBuffer cmd, StagingPool::StagingBuffer * sb);
	};
}