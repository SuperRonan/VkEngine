#include "TransferCommand.hpp"

namespace vkl
{
	BlitImage::BlitImage(CreateInfo const& ci):
		DeviceCommand(ci.app, ci.name),
		_filter(ci.filter)
	{
		setImages(ci.src, ci.dst);
		setRegions(ci.regions);
	}

	void BlitImage::setImages(std::shared_ptr<ImageView> src, std::shared_ptr<ImageView> dst)
	{
		_src = src;
		_dst = dst;

		_resources.resize(0);

		_resources.push_back(Resource{
			._images = {_src},
			._begin_state = ResourceState{
				._access = VK_ACCESS_TRANSFER_READ_BIT,
				._layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				._stage = VK_PIPELINE_STAGE_TRANSFER_BIT
			},
			._image_usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		});
		_resources.push_back(Resource{
			._images = {_dst},
			._begin_state = ResourceState{
				._access = VK_ACCESS_TRANSFER_WRITE_BIT,
				._layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				._stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
			},
			._image_usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		});
	}

	void BlitImage::setRegions(std::vector<VkImageBlit> const& regions)
	{
		_regions = regions;
		if (_regions.empty() && !!_src && !!_dst)
		{
			_regions.resize(1);
			VkImageBlit& region = _regions.front();
			// TODO Correctly manage the extent based on the type 
			// (in the case of a 3D image viewd as a 2D imageArray)
			// TODO manage multi mip blit?
			region = VkImageBlit{
				.srcSubresource = getImageLayersFromRange(_src->range()),
				.srcOffsets = {makeZeroOffset3D(), convert(_src->image()->extent())},
				.dstSubresource = getImageLayersFromRange(_dst->range()),
				.dstOffsets = {makeZeroOffset3D(), convert(_dst->image()->extent())},
			};
		}
	}

	void BlitImage::init()
	{
		
	}

	void BlitImage::execute(ExecutionContext& context)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		recordInputSynchronization(*cmd, context);

		vkCmdBlitImage(*cmd,
			*_src->image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			*_dst->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			(uint32_t)_regions.size(), _regions.data(), _filter
		);

		declareResourcesEndState(context);
	}

	CopyImage::CopyImage(CreateInfo const& ci):
		DeviceCommand(ci.app, ci.name),
		_src(ci.src),
		_dst(ci.dst),
		_regions(ci.regions)
	{}

	void CopyImage::init()
	{
		_resources.push_back(Resource{
			._images = {_src},
			._begin_state = ResourceState{
				._access = VK_ACCESS_TRANSFER_READ_BIT,
				._layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				._stage = VK_PIPELINE_STAGE_TRANSFER_BIT
			},
			._image_usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		});
		_resources.push_back(Resource{
			._images = {_dst},
			._begin_state = ResourceState{
				._access = VK_ACCESS_TRANSFER_WRITE_BIT,
				._layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				._stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
			},
			._image_usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		});

		if (_regions.empty())
		{
			_regions.resize(1);
			VkImageCopy& region = _regions.front();
			VkExtent3D extent;
			// TODO Correctly manage the extent based on the type 
			// (in the case of a 3D image viewd as a 2D imageArray)
			extent = _src->image()->extent();
			// TODO manage multi mip copy
			region = VkImageCopy{
				.srcSubresource = getImageLayersFromRange(_src->range()),
				.srcOffset = makeZeroOffset3D(),
				.dstSubresource = getImageLayersFromRange(_dst->range()),
				.dstOffset = makeZeroOffset3D(),
				.extent = extent,
			};
		}
	}

	void CopyImage::execute(ExecutionContext& context)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		recordInputSynchronization(*cmd, context);

		vkCmdCopyImage(
			*cmd,
			*_src->image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			*_dst->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			(uint32_t)_regions.size(), _regions.data()
		);

		declareResourcesEndState(context);
	}


	CopyBufferToImage::CopyBufferToImage(CreateInfo const& ci) :
		DeviceCommand(ci.app, ci.name),
		_src(ci.src),
		_dst(ci.dst),
		_regions(ci.regions)
	{}

	void CopyBufferToImage::init()
	{
		_resources.push_back(Resource{
			._buffers = {_src},
			._begin_state = ResourceState{
				._access = VK_ACCESS_TRANSFER_READ_BIT,
				._stage = VK_PIPELINE_STAGE_TRANSFER_BIT
			},
			._buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		});
		_resources.push_back(Resource{
			._images = {_dst},
			._begin_state = ResourceState{
				._access = VK_ACCESS_TRANSFER_WRITE_BIT,
				._layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				._stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
			},
			._image_usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		});

		if (_regions.empty())
		{
			_regions.resize(1);
			VkBufferImageCopy& region = _regions.front();
			VkExtent3D extent;
			// TODO Correctly manage the extent based on the type 
			// (in the case of a 3D image viewd as a 2D imageArray)
			extent = _dst->image()->extent();
			// TODO manage multi mip copy
			region = VkBufferImageCopy{
				.bufferOffset = 0,
				.bufferRowLength = _dst->image()->extent().height,
				.bufferImageHeight = _dst->image()->extent().height,
				.imageSubresource = getImageLayersFromRange(_dst->range()),
				.imageOffset = makeZeroOffset3D(),
				.imageExtent = _dst->image()->extent(),
			};
		}
	}

	void CopyBufferToImage::execute(ExecutionContext& context)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		recordInputSynchronization(*cmd, context);

		vkCmdCopyBufferToImage(
			*cmd,
			*_src, *_dst->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			(uint32_t)_regions.size(), _regions.data()
		);

		declareResourcesEndState(context);
	}


	CopyBuffer::CopyBuffer(CreateInfo const& ci) :
		DeviceCommand(ci.app, ci.name),
		_src(ci.src),
		_dst(ci.dst)
	{}

	void CopyBuffer::init()
	{
		_resources = {
			Resource{
				._buffers = {_src},
				._begin_state = ResourceState{
					._access = VK_ACCESS_TRANSFER_READ_BIT,
					._stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
				},
				._buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			},
			Resource{
				._buffers = {_dst},
				._begin_state = ResourceState{
					._access = VK_ACCESS_TRANSFER_WRITE_BIT,
					._stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
				},
				._buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			},
		};
	}

	void CopyBuffer::execute(ExecutionContext& context)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		recordInputSynchronization(*cmd, context);

		// TODO from the ci
		VkBufferCopy region = {
			.srcOffset = 0,
			.dstOffset = 0,
			.size = std::min(_src->size(), _dst->size()),
		};

		vkCmdCopyBuffer(*cmd,
			*_src, *_dst,
			1, &region
		);

		declareResourcesEndState(context);
	}

	FillBuffer::FillBuffer(CreateInfo const& ci) :
		DeviceCommand(ci.app, ci.name),
		_buffer(ci.buffer),
		_begin(ci.begin),
		_size(ci.size),
		_value(ci.value)
	{

	}

	void FillBuffer::init()
	{
		_resources = {
			Resource{
				._buffers = {_buffer},
				._begin_state = ResourceState{
					._access = VK_ACCESS_TRANSFER_WRITE_BIT,
					._stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
				},
				._buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			},
		};
	}

	void FillBuffer::execute(ExecutionContext& context)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		recordInputSynchronization(*cmd, context);

		vkCmdFillBuffer(*cmd, *_buffer, _begin, _size, _value);

		declareResourcesEndState(context);
	}



	ClearImage::ClearImage(CreateInfo const& ci):
		DeviceCommand(ci.app, ci.name),
		_view(ci.view),
		_value(ci.value)
	{

	}

	void ClearImage::init()
	{
		_resources = {
			Resource{
				._images = {_view},
				._begin_state = ResourceState{
					._access = VK_ACCESS_TRANSFER_WRITE_BIT,
					._layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					._stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
				},
				._image_usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			},
		};
	}

	void ClearImage::execute(ExecutionContext& context)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		recordInputSynchronization(*cmd, context);

		const VkImageSubresourceRange range = _view->range();

		// TODO depth clear if applicable

		vkCmdClearColorImage(*cmd, *_view->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &_value.color, 1, &range);

		declareResourcesEndState(context);
	}
}