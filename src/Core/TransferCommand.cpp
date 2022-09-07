#include "TransferCommand.hpp"

namespace vkl
{
	void BlitImage::init()
	{
		_resources.push_back(Resource{
			._images = {_src},
			._beging_state = ResourceState{
				._access = VK_ACCESS_TRANSFER_READ_BIT,
				._layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				._stage = VK_PIPELINE_STAGE_TRANSFER_BIT
			},
		});
		_resources.push_back(Resource{
			._images = {_dst},
			._beging_state = ResourceState{
				._access = VK_ACCESS_TRANSFER_WRITE_BIT,
				._layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				._stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
			},
		});

		if (_regions.empty())
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

	void BlitImage::execute(ExecutionContext& context)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCurrentCommandBuffer();
		recordInputSynchronization(*cmd, context);

		vkCmdBlitImage(*cmd,
			*_src->image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			*_dst->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			(uint32_t)_regions.size(), _regions.data(), _filter
		);

		declareResourcesEndState(context);
	}

	void CopyImage::init()
	{
		_resources.push_back(Resource{
			._images = {_src},
			._beging_state = ResourceState{
				._access = VK_ACCESS_TRANSFER_READ_BIT,
				._layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				._stage = VK_PIPELINE_STAGE_TRANSFER_BIT
			},
		});
		_resources.push_back(Resource{
			._images = {_dst},
			._beging_state = ResourceState{
				._access = VK_ACCESS_TRANSFER_WRITE_BIT,
				._layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				._stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
			},
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
		std::shared_ptr<CommandBuffer> cmd = context.getCurrentCommandBuffer();
		recordInputSynchronization(*cmd, context);

		vkCmdCopyImage(
			*cmd,
			*_src->image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			*_dst->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			(uint32_t)_regions.size(), _regions.data()
		);

		declareResourcesEndState(context);
	}
}