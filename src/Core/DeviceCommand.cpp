#include "DeviceCommand.hpp"

namespace vkl
{
	void DeviceCommand::recordInputSynchronization(CommandBuffer& cmd, ExecutionContext& context)
	{
		std::vector<VkImageMemoryBarrier> image_barriers;
		std::vector<VkBufferMemoryBarrier> buffer_barriers;
		VkPipelineStageFlags src_stage = 0, dst_stage = 0;
		for (size_t i = 0; i < _resources.size(); ++i)
		{
			auto& r = _resources[i];
			const ResourceState next = r._begin_state;
			const ResourceState prev = [&]() {
				if (r.isImage())
				{
					return context.getImageState(r._images[0]);
				}
				else if (r.isBuffer())
				{
					return context.getBufferState(r._buffers[0]);
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
				src_stage |= prev._stage;
				dst_stage |= next._stage;
			}
		}
		if (!image_barriers.empty() || !buffer_barriers.empty())
		{
			if (src_stage == 0)
			{
				src_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			}
			if (dst_stage & VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT)
			{
				int _ = 0;
			}
			vkCmdPipelineBarrier(cmd,
				src_stage, dst_stage, 0,
				0, nullptr,
				(uint32_t)buffer_barriers.size(), buffer_barriers.data(),
				(uint32_t)image_barriers.size(), image_barriers.data());
		}
	}

	void DeviceCommand::declareResourcesEndState(ExecutionContext& context)
	{
		for (const auto& r : _resources)
		{
			ResourceState const& s = r._end_state.value_or(r._begin_state);
			if (r.isBuffer())
			{
				context.setBufferState(r._buffers.front(), s);
			}
			else if (r.isImage())
			{
				context.setImageState(r._images.front(), s);
			}
			else
			{
				assert(false);
			}
		}
	}
}