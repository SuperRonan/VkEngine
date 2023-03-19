#pragma once

#include "VkApplication.hpp"
#include <cassert>
#include <array>
#include <format>
#include "DynamicValue.hpp"

namespace vkl
{
	class ImageInstance : public VkObject
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
		};

		using AI = AssociateInfo;

	protected:

		VkImageCreateInfo _ci = {};
		VmaAllocationCreateInfo _vma_ci = {};

		VmaAllocation _alloc = nullptr;
		VkImage _image = VK_NULL_HANDLE;

		void setVkNameIFP();

		void create();

		void destroy();

	public:

		ImageInstance(CreateInfo const& ci);

		ImageInstance(AssociateInfo const& ci);

		virtual ~ImageInstance();

		ImageInstance(ImageInstance const&) = delete;

		ImageInstance(ImageInstance&&) = delete;

		ImageInstance& operator=(ImageInstance const&) = delete;
		
		ImageInstance& operator=(ImageInstance &&) = delete;

		
		constexpr VkImageCreateInfo const& CreateInfo()const
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

	};

	class Image : public VkObject
	{
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = "";
			VkImageCreateFlags flags = 0;
			VkImageType type = VK_IMAGE_TYPE_MAX_ENUM;
			VkFormat format = VK_FORMAT_MAX_ENUM;
			std::shared_ptr<DynamicValue<VkExtent3D>> extent = nullptr;
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
			std::shared_ptr<DynamicValue<VkExtent3D>> extent;
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

	protected:

		VkImageCreateFlags _flags = 0;
		VkImageType _type = VK_IMAGE_TYPE_MAX_ENUM;
		VkFormat _format = VK_FORMAT_MAX_ENUM;
		std::shared_ptr<DynamicValue<VkExtent3D>> _extent = nullptr;
		uint32_t _mips = 1;
		uint32_t _layers = 1;
		VkSampleCountFlagBits _samples = VK_SAMPLE_COUNT_1_BIT;
		VkImageTiling _tiling = VK_IMAGE_TILING_OPTIMAL;
		VkImageUsageFlags _usage = 0;
		std::vector<uint32_t> _queues = {};
		VkSharingMode _sharing_mode = VK_SHARING_MODE_MAX_ENUM;
		VkImageLayout _initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;

		VmaMemoryUsage _mem_usage = VMA_MEMORY_USAGE_UNKNOWN;

		std::shared_ptr<ImageInstance> _inst = nullptr;
		

	public:

		constexpr Image(VkApplication * app = nullptr) noexcept:
			VkObject(app)
		{}

		Image(CreateInfo const& ci);

		constexpr Image(Image const&) noexcept = delete;

		Image(Image&& other) noexcept :
			VkObject(std::move(other)),
			_flags(other._flags),
			_type(other._type),
			_format(other._format),
			_extent(other._extent),
			_mips(other._mips),
			_layers(other._layers),
			_samples(other._samples),
			_tiling(other._tiling),
			_usage(other._usage),
			_queues(std::move(other._queues)),
			_sharing_mode(other._sharing_mode),
			_mem_usage(other._mem_usage),
			_inst(std::move(other._inst)),
			_initial_layout(other._initial_layout)
		{

		}

		constexpr Image& operator=(Image const&) noexcept = delete;

		constexpr Image& operator=(Image&& other)noexcept
		{
			VkObject::operator=(std::move(other));
			std::swap(_flags, other._flags);
			std::swap(_type, other._type);
			std::swap(_format, other._format);
			std::swap(_extent, other._extent);
			std::swap(_mips, other._mips);
			std::swap(_layers, other._layers);
			std::swap(_samples, other._samples);
			std::swap(_tiling, other._tiling);
			std::swap(_usage, other._usage);
			std::swap(_queues, other._queues);
			std::swap(_sharing_mode, other._sharing_mode);
			std::swap(_mem_usage, other._mem_usage);
			std::swap(_inst, other._inst);
			std::swap(_initial_layout, other._initial_layout);
			return *this;
		}

		~Image() {};

		void createInstance();

		void associateImage(AssociateInfo const& assos);

		void destroyInstance();

		constexpr const std::shared_ptr<ImageInstance>& instance()const
		{
			return _inst;
		}

		constexpr std::shared_ptr<ImageInstance>& instance()
		{
			return _inst;
		}

		operator std::shared_ptr<ImageInstance>()const
		{
			return instance();
		}

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

		std::shared_ptr<DynamicValue<VkExtent3D>> extent()const
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

		//StagingPool::StagingBuffer* copyToStaging2D(StagingPool& pool, void* data, uint32_t elem_size);
	};
}