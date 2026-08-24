#pragma once

#include <vkl/App/VkApplication.hpp>
#include "AbstractInstance.hpp"
#include <vkl/Core/DynamicValue.hpp>
#include <vkl/Execution/UpdateContext.hpp>
#include <atomic>
#include <vkl/Execution/ResourceState.hpp>

namespace vkl
{
	namespace GUI
	{
		class ImageInstanceInspector;
		class ImageInspector;
	}
	class ImageInstance : public InstanceBase<VkImage>
	{
	public:

		static constinit const uint32_t ALL_MIPS = VK_REMAINING_MIP_LEVELS;
		static constinit const uint32_t ALL_LAYERS = VK_REMAINING_ARRAY_LAYERS;

		using Parent = InstanceBase<VkImage>;

		static constexpr const char* ClassName = "Image";

		struct CreateInfo
		{
			VkApplication  * app = nullptr;
			std::string name = {};
			size_t tick = 0;
			VkImageCreateInfo ci;
			VmaAllocationCreateInfo aci;
		};

		using CI = CreateInfo;

		struct AssociateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			size_t tick = 0;
			VkImageCreateInfo ci;
			VkImage image = VK_NULL_HANDLE;
		};

		using AI = AssociateInfo;

		using Range = VkImageSubresourceRange;

	protected:

		friend class SynchronizationHelperV2;

		static std::atomic<size_t> _instance_counter;

		VkImageCreateInfo _ci = {};
		MyVector<uint32_t> _queues = {};
		VmaAllocationCreateInfo _vma_ci = {};

		VmaAllocation _alloc = nullptr;
		size_t _unique_id = 0;
		bool _remaining_mips = false;

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

		void create();

		void destroy();

		void setInitialState(size_t tid);

		bool statesAreSorted(size_t tid) const;

		void setQueues();

		void setMipsCount();

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

		constexpr VkPhysicalDeviceImageFormatInfo2 imageFormatInfo2() const
		{
			VkPhysicalDeviceImageFormatInfo2 info{
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
				.pNext = nullptr,
				.format = _ci.format,
				.type = _ci.imageType,
				.tiling = _ci.tiling,
				.usage = _ci.usage,
				.flags = _ci.flags,
			};
			return info;
		}

		constexpr MyVector<uint32_t> const& queues() const
		{
			return _queues;
		}


		constexpr VmaAllocationCreateInfo const& allocationInfo()const
		{
			return _vma_ci;
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

		constexpr uint32_t mipLevels() const
		{
			return _remaining_mips ? ALL_MIPS : _ci.mipLevels;
		}

		constexpr VkImageSubresourceRange defaultSubresourceRange()const
		{
			return VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, // TODO determine the aspect from the format
				.baseMipLevel = 0,
				.levelCount = mipLevels(),
				.baseArrayLayer = 0,
				.layerCount = _ci.arrayLayers,
			};
		}

		// Make sure the layers_count is correct (array layers for layered images, depth for 3D images)
		static Range FiniteRange(Range const& range, uint32_t layers_count, uint32_t mips_count)
		{
			Range res = range;
			if (res.layerCount == VK_REMAINING_ARRAY_LAYERS)
			{
				res.layerCount = layers_count - res.baseArrayLayer;
			}
			if (res.levelCount == VK_REMAINING_MIP_LEVELS)
			{
				res.levelCount = mips_count - res.baseMipLevel;
			}
			return res;
		}

		Range finiteRange(Range const& range) const
		{
			// I am 99% sure the depth is used are the array dimension when taking 2D views of 3D images
			// But I haven't confirmed it yet
			uint32_t layers = (_ci.imageType == VK_IMAGE_TYPE_3D) ? _ci.extent.depth : _ci.arrayLayers;
			return FiniteRange(range, layers, _ci.mipLevels);
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

		using InspectorType = GUI::ImageInstanceInspector;
		friend class InspectorType;

		virtual std::shared_ptr<GUI::Panel> makeInspector(GUI::InspectorMakeInfo const& imi) override;
	};

	class Image : public InstanceHolder<ImageInstance>
	{
	public:

		using Parent = InstanceHolder<ImageInstance>;

		static constinit const uint32_t ALL_MIPS = ImageInstance::ALL_MIPS;
		static constinit const uint32_t ALL_LAYERS = ImageInstance::ALL_LAYERS;

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
			MyVector<uint32_t> queues = {};
			VmaMemoryUsage mem_usage = VMA_MEMORY_USAGE_GPU_ONLY;
			VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
			bool create_on_construct = false;
			Dyn<bool> hold_instance = true;
		};

		using CI = CreateInfo;

		static constexpr uint32_t HowManyMips(uint32_t dims, VkExtent3D const& extent)
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

		static constexpr uint32_t HowManyMips(VkImageType type, VkExtent3D const& extent)
		{
			return HowManyMips(((uint32_t)type) + 1, extent);
		}

		virtual void updateResourcesInline(UpdateContext& ctx, UpdateResourcesResult& res) override;

		virtual void destroyInstanceIFN() override;

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
		MyVector<uint32_t> _queues = {};
		VkSharingMode _sharing_mode = VK_SHARING_MODE_MAX_ENUM;
		VkImageLayout _initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;

		VmaMemoryUsage _mem_usage = VMA_MEMORY_USAGE_UNKNOWN;

	public:

		Image(CreateInfo const& ci);

		Image(std::shared_ptr<ImageInstance> const& inst);

		virtual ~Image() override {};

		void createInstance(size_t tick = 0);

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

		VkImageSubresourceRange fullSubresourceRange() const
		{
			VkImageAspectFlags aspect = getImageAspectFromFormat(_format.value());
			return VkImageSubresourceRange{
				.aspectMask = aspect,
				.baseMipLevel = 0,
				.levelCount = VK_REMAINING_MIP_LEVELS,
				.baseArrayLayer = 0,
				.layerCount = VK_REMAINING_ARRAY_LAYERS,
			};
		}

		// Watch out for the ownership!
		Dyn<VkImageSubresourceRange> dynFullSubresourceRange() const
		{
			return [this](){return fullSubresourceRange();};
		}

		VkImageSubresourceRange finiteRange(VkImageSubresourceRange const& range) const;

		using InspectorType = GUI::ImageInspector;
		friend class InspectorType;
		
		virtual std::shared_ptr<GUI::Panel> makeInspector(GUI::InspectorMakeInfo const& imi) override;
	};

	VKL_DEFINE_DESCRIPTOR_INSTANCE_POINTERS(Image)
}