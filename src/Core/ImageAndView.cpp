#include "ImageAndView.hpp"

namespace vkl
{
	ImageAndView::ImageAndView(VkApplication * app, const CreateInfo& ci):
		VkObject(app)
	{
		_image = std::make_shared<Image>(app, ci);
		_view = std::make_shared<ImageView>(*_image, ci.aspect);
	}

	ImageAndView::ImageAndView(std::shared_ptr<Image> const& image, std::shared_ptr<ImageView> const& view):
		VkObject(image->application()),
		_image(image),
		_view(view)
	{}

	ImageAndView::ImageAndView(std::shared_ptr<Image> const& image, VkImageAspectFlags aspect) :
		VkObject(image->application()),
		_image(image),
		_view(std::make_shared<ImageView>(*image, aspect))
	{}

	void ImageAndView::reset()
	{
		_view = nullptr;
		_image = nullptr;
	}

	StagingPool::StagingBuffer* ImageAndView::copyToStaging2D(StagingPool& pool, void* data)
	{
		assert(_type == VK_IMAGE_TYPE_2D);
		size_t size = _image->extent().width * _image->extent().height * _image->elemSize();
		StagingPool::StagingBuffer* sb = pool.getStagingBuffer(size);
		std::memcpy(sb->data, data, size);
		return sb;
	}

	void ImageAndView::recordSendStagingToDevice2D(VkCommandBuffer command_buffer, StagingPool::StagingBuffer* sb, VkImageLayout layout)
	{
		VkBufferImageCopy copy{
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = VkImageSubresourceLayers{
				.aspectMask = _view->range().aspectMask,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			.imageOffset = {0, 0, 0},
			.imageExtent = _image->extent(),
		};
		vkCmdCopyBufferToImage(command_buffer, sb->buffer, *_image, layout, 1, &copy);
	}

	void ImageAndView::recordTransitionLayout(VkCommandBuffer command, VkImageLayout src, VkAccessFlags src_access, VkPipelineStageFlags src_stage, VkImageLayout dst, VkAccessFlags dst_access, VkPipelineStageFlags dst_stage)
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
			.subresourceRange = VkImageSubresourceRange{
				.aspectMask = _view->range().aspectMask,
				.baseMipLevel = 0,
				.levelCount = _image->mips(),
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