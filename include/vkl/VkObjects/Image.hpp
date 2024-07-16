#pragma once

#include <vkl/App/VkApplication.hpp>
#include "AbstractInstance.hpp"
#include <cassert>
#include <array>
#include <format>
#include <vkl/Core/DynamicValue.hpp>
#include <vkl/Execution/UpdateContext.hpp>
#include <atomic>
#include <vkl/Execution/ResourceState.hpp>

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

		friend class SynchronizationHelperV2;

		static std::atomic<size_t> _instance_counter;

		VkImageCreateInfo _ci = {};
		VmaAllocationCreateInfo _vma_ci = {};

		VmaAllocation _alloc = nullptr;
		VkImage _image = VK_NULL_HANDLE;
		size_t _unique_id = 0;

		struct InternalStates
		{
			struct PosAndState
			{
				uint32_t pos = 0;
				ResourceState2 write_state = {};
				ResourceState2 read_only_state = {};
			};
			// One per mip level
			//	- Layers (similar to buffer)
			std::vector<std::vector<PosAndState>> states;
		};

		std::HMap<size_t, InternalStates> _states = {};


		void setVkNameIFP();

		void create();

		void destroy();

		void setInitialState(size_t tid);

		bool statesAreSorted(size_t tid) const;

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

		constexpr VkImageSubresourceRange defaultSubresourceRange()const
		{
			return VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, // TODO determine the aspect from the format
				.baseMipLevel = 0,
				.levelCount = _ci.mipLevels,
				.baseArrayLayer = 0,
				.layerCount = _ci.arrayLayers,
			};
		}

		struct StateInRange
		{
			DoubleResourceState2 state;
			Range range;
		};
		
		void fillState(size_t tid, Range const& range, MyVector<StateInRange> & res) const;
		
		MyVector<StateInRange> getState(size_t tid, Range const& range) const
		{
			MyVector<StateInRange> res;
			fillState(tid, range, res);
			return res;
		}

		void setState(size_t tid, Range const& range, ResourceState2 const& state);

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
			Dyn<VkFormat> format;
			Dyn<VkExtent3D> extent;
			uint32_t mips = 1;
			Dyn<uint32_t> layers = 1;
			Dyn<VkSampleCountFlagBits> samples = VK_SAMPLE_COUNT_1_BIT;
			VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
			VkImageUsageFlags usage = 0;
			std::vector<uint32_t> queues = {};
			VmaMemoryUsage mem_usage = VMA_MEMORY_USAGE_GPU_ONLY;
			VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
			bool create_on_construct = false;
			Dyn<bool> hold_instance = true;
		};

		struct AssociateInfo
		{
			std::shared_ptr<ImageInstance> instance;
			Dyn<VkFormat> format;
			Dyn<VkExtent3D> extent;
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
		Dyn<VkFormat> _format;
		Dyn<VkExtent3D> _extent;
		Dyn<uint32_t> _mips = 1; // -1 means all mips possible from resolution
		Dyn<uint32_t> _layers = 1;
		Dyn<VkSampleCountFlagBits> _samples = VK_SAMPLE_COUNT_1_BIT;
		VkImageTiling _tiling = VK_IMAGE_TILING_OPTIMAL;
		VkImageUsageFlags _usage = 0;
		std::vector<uint32_t> _queues = {};
		VkSharingMode _sharing_mode = VK_SHARING_MODE_MAX_ENUM;
		VkImageLayout _initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;

		VmaMemoryUsage _mem_usage = VMA_MEMORY_USAGE_UNKNOWN;

		// Could bitpack maybe
		size_t _latest_update_tick = 0;
		bool _latest_update_res = false;
		bool _inst_all_mips = false;

	public:

		Image(CreateInfo const& ci);

		Image(AssociateInfo const& assos);

		virtual ~Image() override {};

		void createInstance();

		constexpr VkImageCreateFlags flags()const
		{
			return _flags;
		}

		constexpr VkImageType type()const
		{
			return _type;
		}

		constexpr const Dyn<VkFormat>& format()const
		{
			return _format;
		}

		constexpr const Dyn<VkExtent3D>& extent()const
		{
			return _extent;
		}

		constexpr const Dyn<uint32_t>& mips()const
		{
			return _mips;
		}

		uint32_t actualMipsCount()const;

		constexpr const Dyn<uint32_t>& layers()const
		{
			return _layers;
		}

		constexpr const Dyn<VkSampleCountFlagBits>& sampleCount()const
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

		//VkImageSubresourceRange defaultSubresourceRange();

		Dyn<VkImageSubresourceRange> fullSubresourceRange();

		bool updateResource(UpdateContext & ctx);
	};
}