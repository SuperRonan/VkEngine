#include "ImageView.hpp"

namespace vkl
{
	void ImageView::createView()
	{
		VkImageViewCreateInfo ci{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = _image->handle(),
			.viewType = _type,
			.format = _format,
			.components = _components,
			.subresourceRange = _range,
		};

		VK_CHECK(vkCreateImageView(_app->device(), &ci, nullptr, &_view), "Failed to create an image view.");
	}

	void ImageView::destroyView()
	{
		vkDestroyImageView(_app->device(), _view, nullptr);
		_view = VK_NULL_HANDLE;
	}

	ImageView::~ImageView()
	{
		if (handle() != VK_NULL_HANDLE)
		{
			destroyView();
		}
	}

	ImageView::ImageView(CreateInfo const& ci) :
		VkObject((ci.app ? ci.app : ci.image->application()), ci.name),
		_image(ci.image),
		_type(ci.type == VK_IMAGE_TYPE_MAX_ENUM ? getDefaultViewTypeFromImageType(_image->type()) : ci.type),
		_format(ci.format == VK_FORMAT_MAX_ENUM ? _image->format() : ci.format),
		_components(ci.components),
		_range(ci.range.has_value() ? ci.range.value() : _image->defaultSubresourceRange())
	{
		if (ci.create_on_construct)
		{
			createView();
		}
	}


	ImageView::ImageView(ImageView&& other) noexcept :
		VkObject(std::move(other)),
		_image(std::move(other._image)),
		_type(other._type),
		_format(other._format),
		_components(other._components),
		_range(other._range),
		_view(other._view)
	{
		other._view = VK_NULL_HANDLE;
	}

	ImageView::ImageView(std::shared_ptr<Image> image, VkImageAspectFlags aspect) :
		VkObject(*image),
		_image(image),
		_type(getViewTypeFromImageType(image->type())),
		_format(image->format()),
		_components(defaultComponentMapping()),
		_range(VkImageSubresourceRange{
			.aspectMask = aspect,
			.baseMipLevel = 0,
			.levelCount = image->mips(),
			.baseArrayLayer = 0,
			.layerCount = 1,
		})
		{
			createView();
		}

	ImageView::ImageView(Image && image, VkImageAspectFlags aspect) :
		VkObject(image),
		_image(std::make_shared<Image>(std::move(image))),
		_type(getViewTypeFromImageType(image.type())),
		_format(image.format()),
		_components(defaultComponentMapping()),
		_range(VkImageSubresourceRange{
			.aspectMask = aspect,
			.baseMipLevel = 0,
			.levelCount = image.mips(),
			.baseArrayLayer = 0,
			.layerCount = 1,
			})
	{
		createView();
	}

	ImageView& ImageView::operator=(ImageView&& other)
	{
		//std::copySwap(*this, other);
		VkObject::operator=(std::move(other));
		_image = std::move(other._image);
		std::swap(_type, other._type);
		std::swap(_format, other._format);
		std::swap(_components, other._components);
		std::swap(_range, other._range);
		std::swap(_view, other._view);

		return *this;
	}

	//StagingPool::StagingBuffer* ImageView::copyToStaging2D(StagingPool& pool, void* data, uint32_t elem_size)
	//{
	//	assert(_type == VK_IMAGE_TYPE_2D);
	//	size_t size = _image->extent().width * _image->extent().height * elem_size;
	//	StagingPool::StagingBuffer* sb = pool.getStagingBuffer(size);
	//	std::memcpy(sb->data, data, size);
	//	return sb;
	//}

	//void ImageView::recordSendStagingToDevice2D(VkCommandBuffer command_buffer, StagingPool::StagingBuffer* sb, VkImageLayout layout)
	//{
	//	VkBufferImageCopy copy{
	//		.bufferOffset = 0,
	//		.bufferRowLength = 0,
	//		.bufferImageHeight = 0,
	//		.imageSubresource = VkImageSubresourceLayers{
	//			.aspectMask = _range.aspectMask,
	//			.mipLevel = 0,
	//			.baseArrayLayer = 0,
	//			.layerCount = 1,
	//		},
	//		.imageOffset = {0, 0, 0},
	//		.imageExtent = _image->extent(),
	//	};
	//	vkCmdCopyBufferToImage(command_buffer, sb->buffer, *_image, layout, 1, &copy);
	//}

	void ImageView::recordTransitionLayout(VkCommandBuffer command, VkImageLayout src, VkAccessFlags src_access, VkPipelineStageFlags src_stage, VkImageLayout dst, VkAccessFlags dst_access, VkPipelineStageFlags dst_stage)
	{
		VkImageMemoryBarrier barrier{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = src_access,
			.dstAccessMask = dst_access,
			.oldLayout = src,
			.newLayout = dst,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = *_image,
			.subresourceRange = _range,
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