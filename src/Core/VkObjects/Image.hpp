#pragma once

#include <Core/App/VkApplication.hpp>
#include "AbstractInstance.hpp"
#include <cassert>
#include <array>
#include <format>
#include <Core/DynamicValue.hpp>
#include <Core/Execution/UpdateContext.hpp>
#include <atomic>
#include <Core/Execution/ResourceState.hpp>

namespace vkl
{
	class ImageInstance : public AbstractInstance
	{
	public:

		struct CreateInfo
		{
			VkApplication  * app = nullptr;
			std::string name = {};
			VkImageCreateInfo ci;
			VmaAllocationCreateInfo aci;
		};

		using CI = CreateInfo;

		struct AssociateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VkImageCreateInfo ci;
			VkImage image = VK_NULL_HANDLE;
		};

		using AI = AssociateInfo;

		using Range = VkImageSubresourceRange;

	protected:

		static std::atomic<size_t> _instance_counter;

		VkImageCreateInfo _ci = {};
		VmaAllocationCreateInfo _vma_ci = {};

		VmaAllocation _alloc = nullptr;
		VkImage _image = VK_NULL_HANDLE;
		size_t _unique_id = 0;

		struct InternalStates
		{
			mutable std::HMap<Range, ResourceState2> states = {};
		};

		std::HMap<size_t, InternalStates> _states = {};


		void setVkNameIFP();

		void create();

		void destroy();

		void setInitialState(size_t tid);

	public:

		ImageInstance(CreateInfo const& ci);

		ImageInstance(AssociateInfo const& ci);

		virtual ~ImageInstance();

		ImageInstance(ImageInstance const&) = delete;

		ImageInstance(ImageInstance&&) = delete;

		ImageInstance& operator=(ImageInstance const&) = delete;
		
		ImageInstance& operator=(ImageInstance &&) = delete;

		constexpr VkImageCreateInfo const& createInfo()const
		{
			return _ci;
		}

		constexpr VmaAllocationCreateInfo const& AllocationInfo()const
		{
			return _vma_ci;
		}

		constexpr auto handle()const
		{
			return _image;
		}

		constexpr VkImage image()const
		{
			return _image;
		}

		constexpr operator VkImage()const
		{
			return _image;
		}

		constexpr VmaAllocation alloc()const
		{
			return _alloc;
		}

		constexpr bool ownership()const
		{
			return !!_alloc;
		}

		constexpr size_t uniqueId()const
		{
			return _unique_id;
		}

		ResourceState2 getState(size_t tid, Range const& range)const
		{
			assert(_states.contains(tid));
			const InternalStates& is = _states.at(tid);
			if (!is.states.contains(range))
			{
				is.states[range] = ResourceState2();
			}
			return is.states.at(range);
		}

		void setState(size_t tid, Range const& range, ResourceState2 const& state)
		{
			if (!_states.contains(tid))
			{
				_states[tid] = InternalStates{};
			}
			_states[tid].states[range] = state;
		}

	};

	class Image : public InstanceHolder<ImageInstance>
	{
	public:

		constexpr static uint32_t ALL_MIPS = uint32_t(-1);

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = "";
			VkImageCreateFlags flags = 0;
			VkImageType type = VK_IMAGE_TYPE_MAX_ENUM;
			VkFormat format = VK_FORMAT_MAX_ENUM;
			DynamicValue<VkExtent3D> extent;
			uint32_t mips = 1;
			uint32_t layers = 1;
			VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
			VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
			VkImageUsageFlags usage = 0;
			std::vector<uint32_t> queues = {};
			VmaMemoryUsage mem_usage = VMA_MEMORY_USAGE_GPU_ONLY;
			VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
			bool create_on_construct = false;
		};

		struct AssociateInfo
		{
			std::shared_ptr<ImageInstance> instance;
			DynamicValue<VkExtent3D> extent;
		};

		using CI = CreateInfo;
		using AI = AssociateInfo;

		static constexpr uint32_t howManyMips(uint32_t dims, VkExtent3D const& extent)
		{
			uint32_t size = extent.width;
			if (dims == 2)
				size = std::min(size, extent.height);
			if (dims == 3)
				size = std::min(size, extent.depth);
			uint32_t res = 1;
			while (size > 2)
			{
				++res;
				size /= 2;
			}

			return res;
		}

		static constexpr uint32_t howManyMips(VkImageType type, VkExtent3D const& extent)
		{
			return howManyMips(((uint32_t)type) + 1, extent);
		}

		void associateImage(AssociateInfo const& assos);

	protected:

		VkImageCreateFlags _flags = 0;
		VkImageType _type = VK_IMAGE_TYPE_MAX_ENUM;
		VkFormat _format = VK_FORMAT_MAX_ENUM;
		DynamicValue<VkExtent3D> _extent;
		uint32_t _mips = 1;
		uint32_t _layers = 1;
		VkSampleCountFlagBits _samples = VK_SAMPLE_COUNT_1_BIT;
		VkImageTiling _tiling = VK_IMAGE_TILING_OPTIMAL;
		VkImageUsageFlags _usage = 0;
		std::vector<uint32_t> _queues = {};
		VkSharingMode _sharing_mode = VK_SHARING_MODE_MAX_ENUM;
		VkImageLayout _initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;

		VmaMemoryUsage _mem_usage = VMA_MEMORY_USAGE_UNKNOWN;


	public:

		Image(CreateInfo const& ci);

		Image(AssociateInfo const& assos);

		virtual ~Image() override {};

		void createInstance();

		

		void destroyInstance();

		constexpr VkImageCreateFlags flags()const
		{
			return _flags;
		}

		constexpr VkImageType type()const
		{
			return _type;
		}

		constexpr VkFormat format()const
		{
			return _format;
		}

		DynamicValue<VkExtent3D> extent()const
		{
			return _extent;
		}

		constexpr uint32_t mips()const
		{
			return _mips;
		}

		constexpr uint32_t layers()const
		{
			return _layers;
		}

		constexpr VkSampleCountFlagBits sampleCount()const
		{
			return _samples;
		}

		constexpr VkImageTiling tiling()const
		{
			return _tiling;
		}

		constexpr VkImageUsageFlags usage()const
		{
			return _usage;
		}

		constexpr VkSharingMode sharingMode()const
		{
			return _sharing_mode;
		}

		constexpr const std::vector<uint32_t>& queues()const
		{
			return _queues;
		}

		constexpr VkImageLayout initialLayout()const
		{
			return _initial_layout;
		}

		constexpr VkImageSubresourceRange defaultSubresourceRange()const
		{
			return VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, // TODO determine the aspect from the format
				.baseMipLevel = 0,
				.levelCount = _mips,
				.baseArrayLayer = 0,
				.layerCount = _layers,
			};
		}

		bool updateResource(UpdateContext & ctx);

		//StagingPool::StagingBuffer* copyToStaging2D(StagingPool& pool, void* data, uint32_t elem_size);
	};
}