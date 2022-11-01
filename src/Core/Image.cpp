#include "Image.hpp"

namespace vkl
{
	Image::Image(CreateInfo const& ci) : 
		VkObject(ci.app, ci.name),
		_flags(ci.flags),
		_type(ci.type),
		_format(ci.format),
		_extent(ci.extent),
		_mips(ci.mips == uint32_t(-1) ? Image::howManyMips(ci.type, ci.extent) : ci.mips),
		_layers(ci.layers),
		_samples(ci.samples),
		_tiling(ci.tiling),
		_usage(ci.usage),
		_sharing_mode(ci.queues.size() <= 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT),
		_queues(ci.queues),
		_initial_layout(ci.initial_layout),
		_mem_usage(ci.mem_usage)
	{
		if(ci.create_on_construct)
			create();
	}

	void Image::create()
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
			.flags = _flags,
			.imageType = _type,
			.format = _format,
			.extent = _extent,
			.mipLevels = _mips,
			.arrayLayers = _layers,
			.samples = _samples,
			.tiling = _tiling,
			.usage = _usage,
			.sharingMode = _sharing_mode,
			.queueFamilyIndexCount = n_queues,
			.pQueueFamilyIndices = p_queues,
			.initialLayout = _initial_layout,
		};

		VmaAllocationCreateInfo alloc_ci{
			.usage = _mem_usage,
		};

		VK_CHECK(vmaCreateImage(_app->allocator(), &image_ci, &alloc_ci, &_image, &_alloc, nullptr), "Failed to create an image.");
		
		if (!name().empty())
		{
			VkDebugUtilsObjectNameInfoEXT object_name = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext = nullptr,
				.objectType = VK_OBJECT_TYPE_IMAGE,
				.objectHandle = (uint64_t) _image,
				.pObjectName = _name.data(),
			};
			_app->nameObject(object_name);
		}
	}

	void Image::associateImage(AssociateInfo const& assos)
	{
		_app = assos.app;
		_name = assos.name;

		_image = assos.image;
		_type = assos.type;
		_format = assos.format;
		_extent = assos.extent;
		_mips = assos.mips;
		_layers = 1; // TODO Check if the swapchain can support multi layered images (VR)
		_samples = assos.samples;
		_usage = assos.usage;
		_queues = assos.queues;
		_sharing_mode = (_queues.size() <= 1) ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
		_mem_usage = assos.mem_usage;
		_initial_layout = VK_IMAGE_LAYOUT_UNDEFINED; // In which layout are created swapchain images?
		if (!assos.name.empty())
			_name = assos.name;
		_alloc = nullptr;

		if (!name().empty())
		{
			VkDebugUtilsObjectNameInfoEXT object_name = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext = nullptr,
				.objectType = VK_OBJECT_TYPE_IMAGE,
				.objectHandle = (uint64_t)_image,
				.pObjectName = _name.data(),
			};
			_app->nameObject(object_name);
		}
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
		if (ownership())
		{
			vmaDestroyImage(_app->allocator(), _image, _alloc);
		}

		_image = VK_NULL_HANDLE;
		_alloc = nullptr;
	}

	//StagingPool::StagingBuffer* Image::copyToStaging2D(StagingPool& pool, void* data, uint32_t elem_size)
	//{
	//	assert(_type == VK_IMAGE_TYPE_2D);
	//	size_t size = _extent.width * _extent.height * elem_size;
	//	StagingPool::StagingBuffer* sb = pool.getStagingBuffer(size);
	//	std::memcpy(sb->data, data, size);
	//	return sb;
	//}
}