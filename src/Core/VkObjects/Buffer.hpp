#pragma once

#include <Core/App/VkApplication.hpp>
#include <set>
#include <Core/Utils/stl_extension.hpp>
#include "AbstractInstance.hpp"
#include <Core/Execution/UpdateContext.hpp>
#include <atomic>
#include <Core/Execution/ResourceState.hpp>

#ifndef VMA_NULL
#define VMA_NULL nullptr
#endif

namespace vkl
{
	class BufferInstance : public AbstractInstance
	{
	public:

		using Range = Range_st;

	protected:

		friend class SynchronizationHelperV2;

		static std::atomic<size_t> _instance_counter;

		VkBufferCreateInfo _ci;
		VmaAllocationCreateInfo  _aci;
		// The VkBuffer handle is not unique (it can be the same as one used before when recreating the instance)
		VkBuffer _buffer = VK_NULL_HANDLE;
		size_t _unique_id = 0;
		VmaAllocator _allocator = VMA_NULL;
		VmaAllocation _alloc = VMA_NULL;

		VkDeviceAddress _address = 0;

		struct InternalStates
		{
			struct PosAndState
			{
				size_t pos = 0;
				ResourceState2 write_state = {};
				ResourceState2 read_only_state = {};
			};
			// Sorted by pos
			std::vector<PosAndState> states;
		};

		std::HMap<size_t, InternalStates> _states = {};

		void* _data = nullptr;

		void create();

		void setVkName();

		void destroy();

		bool statesAreSorted(size_t tid) const;

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

		void* map();

		void unMap();

		void flush();

		VkDeviceAddress deviceAddress() const
		{
			assert(_ci.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
			return _address;
		}

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

		constexpr size_t uniqueId()const
		{
			return _unique_id;
		}

		Range fullRange()const
		{
			return Range{
				.begin = 0,
				.len = _ci.size,
			};
		}

		struct ResourceKey
		{
			size_t id = 0;
			Range range;
		};

		constexpr ResourceKey getResourceKey(Range const& r)const
		{
			return ResourceKey{
				.id = _unique_id,
				.range = r,
			};
		}

		constexpr ResourceKey getResourceKey() const
		{
			return getResourceKey(Range{.begin = 0, .len = _ci.size, });
		}

		DoubleDoubleResourceState2 getState(size_t tid, Range range)const;

		void setState(size_t tid, Range range, ResourceState2 const& state);

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

			Dyn<bool> hold_instance = true;
		};
		using CI = CreateInfo;

	protected:

		DynamicValue<VkDeviceSize> _size = 0;
		VkBufferUsageFlags _usage = 0;
		std::vector<uint32_t> _queues = {};
		VkSharingMode _sharing_mode = VK_SHARING_MODE_MAX_ENUM;
		VmaMemoryUsage _mem_usage = VMA_MEMORY_USAGE_MAX_ENUM;
		VmaAllocator _allocator = nullptr;
		

	public:

		using Range = typename BufferInstance::Range;

		Buffer(CreateInfo const& ci);

		virtual ~Buffer() override;

		constexpr const DynamicValue<VkDeviceSize> & size()const
		{
			return _size;
		}

		constexpr DynamicValue<VkDeviceSize>& size()
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

		DynamicValue<Range> fullRange() const
		{
			return [this](){
				return Range{
					.begin = 0, 
					.len = size().value(),
				};
			};
		}

		void createInstance();

		bool updateResource(UpdateContext & ctx);

		//BufferPool::PooledBuffer * copyToStaging(void* data, size_t size = 0);

		//void recordCopyStagingToBuffer(VkCommandBuffer cmd, BufferPool::PooledBuffer * sb);
	};

	struct BufferAndRangeInstance
	{
		std::shared_ptr<BufferInstance> buffer = {};
		Buffer::Range range = {};

		operator bool()const
		{
			return buffer.operator bool();
		}

		VkDeviceAddress deviceAddress() const
		{
			VkDeviceAddress res = 0;
			if(buffer)	res = buffer->deviceAddress() + range.begin;
			return res;
		}

		size_t size() const
		{
			size_t res = range.len;
			if(res == 0)	res = (buffer->createInfo().size - range.begin);
			return res;
		}
	};
	using BufferSegmentInstance = BufferAndRangeInstance;

	struct BufferAndRange
	{
		using InstanceType = BufferAndRangeInstance;
		std::shared_ptr<Buffer> buffer = {};
		Dyn<Buffer::Range> range = {};

		BufferAndRangeInstance getInstance() const
		{
			BufferAndRangeInstance res;
			if(buffer)	res.buffer = buffer->instance();
			if(range.hasValue())	res.range = range.value();
			return res;
		}

		operator bool()const
		{
			return buffer.operator bool();
		}
	};
	using BufferSegment = BufferAndRange;
}