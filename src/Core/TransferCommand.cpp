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
		});
		_resources.push_back(Resource{
			._images = {_dst},
			._begin_state = ResourceState{
				._access = VK_ACCESS_TRANSFER_WRITE_BIT,
				._layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				._stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
			},
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
		});
		_resources.push_back(Resource{
			._images = {_dst},
			._begin_state = ResourceState{
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

	PrepareForPresetation::PrepareForPresetation(CreateInfo const& ci):
		DeviceCommand(ci.app, ci.name),
		_images(ci.images)
	{}

	void PrepareForPresetation::init()
	{

	}

	void PrepareForPresetation::execute(ExecutionContext& context)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();

		std::shared_ptr<ImageView> view = _images[_present_index];

		const ResourceState prev_state = context.getImageState(*view);

		const ResourceState wanted_state{
			._access = VK_ACCESS_MEMORY_READ_BIT,
			._layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			._stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		};

		if (stateTransitionRequiresSynchronization(prev_state, wanted_state, true))
		{
			VkImageMemoryBarrier barrier {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = prev_state._access,
				.dstAccessMask = wanted_state._access,
				.oldLayout = prev_state._layout,
				.newLayout = wanted_state._layout,
				.srcQueueFamilyIndex = 0,
				.dstQueueFamilyIndex = 0,
				.image = *view->image(),
				.subresourceRange = view->range(),
			};

			vkCmdPipelineBarrier(*cmd, prev_state._stage, wanted_state._stage, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);


		}
		const ResourceState end_state{
			._access = VK_ACCESS_MEMORY_READ_BIT,
			._layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			._stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		};
		context.setImageState(*view, end_state);
	}
}