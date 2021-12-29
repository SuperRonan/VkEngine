#include "Image.hpp"

namespace vkl
{
	Image::Image(VkApplication* app, CreateInfo const& ci, VkImageLayout layout) : 
		VkObject(app),
		_type(ci.type),
		_format(ci.format),
		_extent(ci.extent),
		_mips(ci.use_mips ? Image::howManyMips(ci.type, ci.extent) : 1),
		_samples(ci.samples),
		_tiling(ci.tiling),
		_usage(ci.usage),
		_sharing_mode(ci.queues.size() <= 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT),
		_queues(ci.queues),
		_mem_usage(ci.mem_usage),
		_elem_size(ci.elem_size)
	{
		createImage(layout);
	}

	void Image::createImage(CreateInfo const& ci, VkImageLayout layout)
	{
		assert(_image == VK_NULL_HANDLE);
		_type = ci.type;
		_format = ci.format;
		_extent = ci.extent;
		_mips = ci.use_mips ? Image::howManyMips(ci.type, ci.extent) : 1;
		_samples = ci.samples;
		_tiling = ci.tiling;
		_usage = ci.usage;
		_sharing_mode = (ci.queues.size() <= 1) ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
		_queues = ci.queues;
		_mem_usage = ci.mem_usage;
		_elem_size = ci.elem_size;

		createImage(layout);
	}

	void Image::createImage(VkImageLayout layout)
	{
		assert(_image == VK_NULL_HANDLE);
		uint32_t n_queues = 0;
		uint32_t* p_queues = nullptr;
		if (_sharing_mode == VK_SHARING_MODE_CONCURRENT)
		{
			n_queues = _queues.size();
			p_queues = _queues.data();
		}

		VkImageCreateInfo image_ci{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.imageType = _type,
			.format = _format,
			.extent = _extent,
			.mipLevels = _mips,
			.arrayLayers = 1,
			.samples = _samples,
			.tiling = _tiling,
			.usage = _usage,
			.sharingMode = _sharing_mode,
			.queueFamilyIndexCount = n_queues,
			.pQueueFamilyIndices = p_queues,
			.initialLayout = layout,
		};

		VmaAllocationCreateInfo alloc_ci{
			.usage = _mem_usage,
		};

		VK_CHECK(vmaCreateImage(_app->allocator(), &image_ci, &alloc_ci, &_image, &_alloc, nullptr), "Failed to create an image.");
		//VK_CHECK(vkCreateImage(_app->device(), &image_ci, nullptr, &_image), "Failed to create an image.");

		_own = true;
	}

	Image::~Image()
	{
		if (_image != VK_NULL_HANDLE)
		{
			destroyImage();
		}
	}

	void Image::destroyImage()
	{
		assert(_image != VK_NULL_HANDLE);
		if (_own)
		{
			vmaDestroyImage(_app->allocator(), _image, _alloc);
		}

		_image = VK_NULL_HANDLE;
		_alloc = nullptr;
	}
}