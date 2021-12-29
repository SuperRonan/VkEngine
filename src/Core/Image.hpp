#pragma once

#include "VkApplication.hpp"
#include <cassert>
#include <array>

namespace vkl
{
	class Image : public VkObject
	{
	public:

		struct CreateInfo
		{
			VkImageType type;
			VkFormat format;
			VkExtent3D extent;
			bool use_mips = true;
			VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
			VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
			VkImageUsageFlags usage;
			std::vector<uint32_t> queues;
			VmaMemoryUsage mem_usage = VMA_MEMORY_USAGE_GPU_ONLY;
			uint32_t elem_size;
		};

		struct AssociateInfo
		{
			VkImage image;
			VkImageType type;
			VkFormat format;
			VkExtent3D extent;
			uint32_t mips = 1;
			VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
			VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
			VkImageUsageFlags usage;
			std::vector<uint32_t> queues;
			VmaMemoryUsage mem_usage = VMA_MEMORY_USAGE_GPU_ONLY;
			uint32_t elem_size;
		};

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

		VkImageType _type;
		VkFormat _format;
		VkExtent3D _extent;
		uint32_t _mips;
		VkSampleCountFlagBits _samples;
		VkImageTiling _tiling;
		VkImageUsageFlags _usage;
		std::vector<uint32_t> _queues;
		VkSharingMode _sharing_mode;

		VmaMemoryUsage _mem_usage;

		uint32_t _elem_size;

		VmaAllocation _alloc = nullptr;
		VkImage _image = VK_NULL_HANDLE;

		bool _own = false;

	public:

		constexpr Image(VkApplication * app = nullptr) noexcept:
			VkObject(app)
		{}

		Image(VkApplication* app, CreateInfo const& ci, VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED);

		constexpr Image(Image const&) noexcept = delete;

		constexpr Image(Image&& other) noexcept :
			_type(other._type),
			_format(other._format),
			_extent(other._extent),
			_samples(other._samples),
			_tiling(other._tiling),
			_usage(other._usage),
			_queues(std::move(other._queues)),
			_sharing_mode(other._sharing_mode),
			_mem_usage(other._mem_usage),
			_elem_size(other._elem_size),
			_alloc(other._alloc),
			_image(other._image),
			_own(other._own)
		{
			other._alloc = nullptr;
			other._image = VK_NULL_HANDLE;
			other._own = false;
		}

		constexpr Image& operator=(Image const&) noexcept = delete;

		constexpr Image& operator=(Image&& other)noexcept
		{
			std::copySwap(*this, other);
			return *this;
		}

		~Image();
		
		void createImage(CreateInfo const& ci, VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED);

		void createImage(VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED);

		constexpr void associateImage(AssociateInfo const& assos)
		{
			_image = assos.image;
			_type = assos.type;
			_format = assos.format;
			_extent = assos.extent;
			_mips = assos.mips;
			_samples = assos.samples;
			_usage = assos.usage;
			_queues = assos.queues;
			_sharing_mode = (_queues.size() <= 1) ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
			_mem_usage = assos.mem_usage;
			_elem_size = assos.elem_size;
			_alloc = nullptr;
			_own = false;
		}

		void destroyImage();

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

		constexpr VkImageType type()const
		{
			return _type;
		}

		constexpr VkFormat format()const
		{
			return _format;
		}

		constexpr VkExtent3D extent()const
		{
			return _extent;
		}

		constexpr uint32_t mips()const
		{
			return _mips;
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

		constexpr bool ownership()const
		{
			return _own;
		}

		constexpr uint32_t elemSize()const
		{
			return _elem_size;
		}
	};
}