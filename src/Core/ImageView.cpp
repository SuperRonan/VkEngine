#include "ImageView.hpp"

namespace vkl
{

	void ImageViewInstance::create()
	{
		_ci.image = *_image;
		VK_CHECK(vkCreateImageView(_app->device(), &_ci, nullptr, &_view), "Failed to create an image view.");

		setVkNameIFP();
	}

	void ImageViewInstance::setVkNameIFP()
	{
		if (!name().empty())
		{
			VkDebugUtilsObjectNameInfoEXT view_name = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext = nullptr,
				.objectType = VK_OBJECT_TYPE_IMAGE_VIEW,
				.objectHandle = (uint64_t)_view,
				.pObjectName = name().c_str(),
			};
			_app->nameObject(view_name);
		}
	}

	void ImageViewInstance::destroy()
	{
		assert(!!_view);
		vkDestroyImageView(_app->device(), _view, nullptr);
		_view = VK_NULL_HANDLE;
		_image = nullptr;
	}

	ImageViewInstance::ImageViewInstance(CreateInfo const& ci):
		VkObject(ci.app, ci.name),
		_image(ci.image),
		_ci(ci.ci)
	{
		create();
	}

	ImageViewInstance::~ImageViewInstance()
	{
		if (!!_view)
		{
			destroy();
		}
	}


	void ImageView::createInstance()
	{
		if (_inst)
		{
			destroyInstance();
		}
		VkImageViewCreateInfo ci{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.viewType = _type,
			.format = _format,
			.components = _components,
			.subresourceRange = _range,
		};
		
		_inst = std::make_shared<ImageViewInstance>(ImageViewInstance::CI{
			.app = application(),
			.name = name(),
			.image = _image->instance(),
			.ci = ci,
		});

		_image->addInvalidationCallback(InvalidationCallback{
			.callback = [&]()
			{
				this->destroyInstance();
			},
			.id = this,	
		});
	}

	void ImageView::destroyInstance()
	{
		if (_inst)
		{
			callInvalidationCallbacks();
			_inst = nullptr;
		}
	}

	ImageView::~ImageView()
	{
		destroyInstance();
	}

	ImageView::ImageView(CreateInfo const& ci) :
		VkObjectWithCallbacks((ci.app ? ci.app : ci.image->application()), ci.name),
		_image(ci.image),
		_type(ci.type == VK_IMAGE_TYPE_MAX_ENUM ? getDefaultViewTypeFromImageType(_image->type()) : ci.type),
		_format(ci.format == VK_FORMAT_MAX_ENUM ? _image->format() : ci.format),
		_components(ci.components),
		_range(ci.range.has_value() ? ci.range.value() : _image->defaultSubresourceRange())
	{
		if (ci.create_on_construct)
		{
			createInstance();
		}
	}


	ImageView::ImageView(ImageView&& other) noexcept :
		VkObjectWithCallbacks(std::move(other)),
		_image(std::move(other._image)),
		_type(other._type),
		_format(other._format),
		_components(other._components),
		_range(other._range),
		_inst(std::move(other._inst))
	{

	}

	//ImageView::ImageView(std::shared_ptr<Image> image, VkImageAspectFlags aspect) :
	//	VkObject(*image),
	//	_image(image),
	//	_type(getDefaultViewTypeFromImageType(image->type())),
	//	_format(image->format()),
	//	_components(defaultComponentMapping()),
	//	_range(VkImageSubresourceRange{
	//		.aspectMask = aspect,
	//		.baseMipLevel = 0,
	//		.levelCount = image->mips(),
	//		.baseArrayLayer = 0,
	//		.layerCount = 1,
	//	})
	//	{
	//		createView();
	//	}

	//ImageView::ImageView(Image && image, VkImageAspectFlags aspect) :
	//	VkObject(image),
	//	_image(std::make_shared<Image>(std::move(image))),
	//	_type(getDefaultViewTypeFromImageType(image.type())),
	//	_format(image.format()),
	//	_components(defaultComponentMapping()),
	//	_range(VkImageSubresourceRange{
	//		.aspectMask = aspect,
	//		.baseMipLevel = 0,
	//		.levelCount = image.mips(),
	//		.baseArrayLayer = 0,
	//		.layerCount = 1,
	//		})
	//{
	//	createView();
	//}

	ImageView& ImageView::operator=(ImageView&& other) noexcept
	{
		//std::copySwap(*this, other);
		VkObjectWithCallbacks::operator=(std::move(other));
		_image = std::move(other._image);
		std::swap(_type, other._type);
		std::swap(_format, other._format);
		std::swap(_components, other._components);
		std::swap(_range, other._range);
		std::swap(_inst, other._inst);

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
			.image = *_inst->image(),
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

	bool ImageView::updateResource()
	{
		const bool updated = _image->updateResource();                      
		
		if (!_inst)
		{
			createInstance();
			return true;
		}

		return false;
	}

}