#include "TransferCommand.hpp"

namespace vkl
{
	void BlitImage::init()
	{
		_resources.push_back(Resource{
			._images = {_src},
			._state = ResourceState{
				._access = VK_ACCESS_TRANSFER_READ_BIT,
				._layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				._stage = VK_PIPELINE_STAGE_TRANSFER_BIT
			},
			});
		_resources.push_back(Resource{
			._images = {_dst},
			._state = ResourceState{
				._access = VK_ACCESS_TRANSFER_WRITE_BIT,
				._layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				._stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
			},
			});
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
	}
}