#include "Texture.hpp"
#include "Image.hpp"


namespace vkl
{

	TextureBase::TextureBase(VkApplication * app, CreateInfo const& ci, VkImageLayout layout):
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
		_view_type(ci.view_type),
		_aspect(ci.aspect),
		_elem_size(ci.elem_size)
	{
		createImage(layout);
		createView();
	}

	void TextureBase::create(CreateInfo const& ci, VkImageLayout layout)
	{
		assert(_image == VK_NULL_HANDLE);
		assert(_view == VK_NULL_HANDLE);

		_type = ci.type;
		_format = ci.format;
		_extent = ci.extent;
		_mips = ci.use_mips ? Image::howManyMips(ci.type, ci.extent) : 1;
		_samples = ci.samples;
		_tiling = ci.tiling;
		_usage = ci.usage;
		_sharing_mode = (ci.queues.size() <= 1) ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
		_queues = ci.queues;
		_view_type = ci.view_type;
		_aspect = ci.aspect;
		_elem_size = ci.elem_size;

		createImage(layout);
		createView();
	}

	TextureBase::~TextureBase()
	{
		if (_view)
			destroyView();
		if (_image)
			destroyImage();
	}

	void TextureBase::createImage(VkImageLayout layout)
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
			.usage = VMA_MEMORY_USAGE_GPU_ONLY,
		};

		VK_CHECK(vmaCreateImage(_app->allocator(), &image_ci, &alloc_ci, &_image, &_alloc, nullptr), "Failed to create an image.");
		//VK_CHECK(vkCreateImage(_app->device(), &image_ci, nullptr, &_image), "Failed to create an image.");
	}

	void TextureBase::createView()
	{
		assert(_view == VK_NULL_HANDLE);
		assert(_image != VK_NULL_HANDLE);

		VkImageViewCreateInfo view_ci{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = _image,
			.viewType = _view_type,
			.format = _format,
			.components = VK_COMPONENT_SWIZZLE_IDENTITY,
			.subresourceRange = VkImageSubresourceRange{
				.aspectMask = _aspect,
				.baseMipLevel = 0,
				.levelCount = _mips,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};

		VK_CHECK(vkCreateImageView(_app->device(), &view_ci, nullptr, &_view), "Failed to create an image view.");
	}

	void TextureBase::destroyImage()
	{
		assert(_image != VK_NULL_HANDLE);

		vmaDestroyImage(_app->allocator(), _image, _alloc);
		_image = VK_NULL_HANDLE;
	}

	void TextureBase::destroyView()
	{
		assert(_view != VK_NULL_HANDLE);

		vkDestroyImageView(_app->device(), _view, nullptr);
		_view = VK_NULL_HANDLE;
	}

	void TextureBase::destroy()
	{
		assert(_image != VK_NULL_HANDLE);
		assert(_view != VK_NULL_HANDLE);

		destroyView();
		destroyImage();
	}

	StagingPool::StagingBuffer* TextureBase::copyToStaging2D(StagingPool& pool, void* data)
	{
		assert(_type == VK_IMAGE_TYPE_2D);
		size_t size = _extent.width * _extent.height * _elem_size;
		StagingPool::StagingBuffer* sb = pool.getStagingBuffer(size);
		std::memcpy(sb->data, data, size);
		return sb;
	}

	void TextureBase::recordSendStagingToDevice2D(VkCommandBuffer command_buffer, StagingPool::StagingBuffer* sb, VkImageLayout layout)
	{
		VkBufferImageCopy copy{
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = VkImageSubresourceLayers{
				.aspectMask = _aspect,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			.imageOffset = {0, 0, 0},
			.imageExtent = _extent,
		};
		vkCmdCopyBufferToImage(command_buffer, sb->buffer, _image, layout, 1, &copy);
	}

	void TextureBase::recordTransitionLayout(VkCommandBuffer command, VkImageLayout prev, VkImageLayout next)
	{
		VkPipelineStageFlags src_stage, dst_stage;
		VkAccessFlags src_access, dst_access;

		switch (prev)
		{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			src_access = 0;
			src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; // Why not bottom?, because we dont care?
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			src_access = VK_ACCESS_TRANSFER_WRITE_BIT;
			src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			src_access = VK_ACCESS_TRANSFER_READ_BIT;
			src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; // Or transfer?
			break;
		default:
			throw std::runtime_error("Unsupported previous image layout for layout transition.");
			break;
		}

		switch (next)
		{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			dst_access = 0;
			dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			dst_access = VK_ACCESS_TRANSFER_WRITE_BIT;
			dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			dst_access = VK_ACCESS_SHADER_READ_BIT;
			dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;
		case VK_IMAGE_LAYOUT_GENERAL:
			dst_access = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			break;
		default:
			throw std::runtime_error("Unsupported next image layout for layout transition.");
			break;
		}

		recordTransitionLayout(command, prev, src_access, src_stage, next, dst_access, dst_stage);
	}

	void TextureBase::recordTransitionLayout(VkCommandBuffer command, VkImageLayout src, VkAccessFlags src_access, VkPipelineStageFlags src_stage, VkImageLayout dst, VkAccessFlags dst_access, VkPipelineStageFlags dst_stage)
	{
		VkImageMemoryBarrier barrier{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = src_access,
			.dstAccessMask = dst_access,
			.oldLayout = src,
			.newLayout = dst,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = _image,
			.subresourceRange = VkImageSubresourceRange{
				.aspectMask = _aspect,
				.baseMipLevel = 0,
				.levelCount = _mips,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};

		vkCmdPipelineBarrier(command,
			src_stage, dst_stage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}
}