#include "DeviceCommand.hpp"

namespace vkl
{
	void DeviceCommand::recordInputSynchronization(CommandBuffer& cmd, ExecutionContext& context)
	{
		std::vector<VkImageMemoryBarrier> image_barriers;
		std::vector<VkBufferMemoryBarrier> buffer_barriers;
		for (size_t i = 0; i < _resources.size(); ++i)
		{
			auto& r = _resources[i];
			const ResourceState next = r._state;
			const ResourceState prev = [&]() {
				if (r.isImage())
				{
					return context.getImageState(*r._images[0].get());
				}
				else if (r.isBuffer())
				{
					return context.getBufferState(*r._buffers[0].get());
				}
				else
				{
					assert(false);
					// ???
					return ResourceState{};
				}
			}();
			if (stateTransitionRequiresSynchronization(prev, next, r.isImage()))
			{
				if (r.isImage())
				{
					VkImageMemoryBarrier barrier = {
						.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						.pNext = nullptr,
						.srcAccessMask = prev._access,
						.dstAccessMask = next._access,
						.oldLayout = prev._layout,
						.newLayout = next._layout,
						.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.image = *r._images[0]->image(),
						.subresourceRange = r._images[0]->range(),
					};
					image_barriers.push_back(barrier);
				}
				else if (r.isBuffer())
				{
					VkBufferMemoryBarrier barrier = {
						.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
						.pNext = nullptr,
						.srcAccessMask = prev._access,
						.dstAccessMask = next._access,
						.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.buffer = *r._buffers[0],
						.offset = 0,
						.size = VK_WHOLE_SIZE,
					};
					buffer_barriers.push_back(barrier);
				}
			}
		}
		if (!image_barriers.empty() || !buffer_barriers.empty())
		{
			vkCmdPipelineBarrier(cmd,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
				0, nullptr,
				(uint32_t)buffer_barriers.size(), buffer_barriers.data(),
				(uint32_t)image_barriers.size(), image_barriers.data());
		}
	}
}