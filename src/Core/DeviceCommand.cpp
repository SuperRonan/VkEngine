#include "DeviceCommand.hpp"

namespace vkl
{
	void InputSynchronizationHelper::addSynch(const Resource& r)
	{
		_resources.push_back(r);
		const ResourceState2 next = r._begin_state;
		const ResourceState2 prev = [&]() {
			if (r.isImage())
			{
				return _ctx.getImageState(r._image);
			}
			else if (r.isBuffer())
			{
				return _ctx.getBufferState(r._buffer);
			}
			else
			{
				assert(false);
				// ???
				return ResourceState2{};
			}
		}();
		if (stateTransitionRequiresSynchronization2(prev, next, r.isImage()))
		{
			if (r.isImage())
			{
				VkImageMemoryBarrier2 barrier = {
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
					.pNext = nullptr,
					.srcStageMask = prev._stage,
					.srcAccessMask = prev._access,
					.dstStageMask = next._stage,
					.dstAccessMask = next._access,
					.oldLayout = prev._layout,
					.newLayout = next._layout,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = *r._image->image()->instance(),
					.subresourceRange = r._image->range(),
				};
				_images_barriers.push_back(barrier);
			}
			else if (r.isBuffer())
			{
				VkBufferMemoryBarrier2 barrier = {
					.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
					.pNext = nullptr,
					.srcStageMask = prev._stage,
					.srcAccessMask = prev._access,
					.dstStageMask = next._stage,
					.dstAccessMask = next._access,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.buffer = *r._buffer->instance(),
					.offset = 0,
					.size = VK_WHOLE_SIZE,
				};
				_buffers_barriers.push_back(barrier);
			}
		}
	}

	void InputSynchronizationHelper::record()
	{
		if (!_images_barriers.empty() || !_buffers_barriers.empty())
		{
			const VkDependencyInfo dep{
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.pNext = nullptr,
				.dependencyFlags = 0,
				.memoryBarrierCount = 0,
				.pMemoryBarriers = nullptr,
				.bufferMemoryBarrierCount = static_cast<uint32_t>(_buffers_barriers.size()),
				.pBufferMemoryBarriers = _buffers_barriers.data(),
				.imageMemoryBarrierCount = static_cast<uint32_t>(_images_barriers.size()),
				.pImageMemoryBarriers = _images_barriers.data(),
			};
			vkCmdPipelineBarrier2(*_ctx.getCommandBuffer(), &dep);
		}
	}

	void InputSynchronizationHelper::NotifyContext()
	{
		for (const auto& r : _resources)
		{
			ResourceState2 const& s = r._end_state.value_or(r._begin_state);
			if (r.isBuffer())
			{
				_ctx.setBufferState(r._buffer, s);
				_ctx.keppAlive(r._buffer);
			}
			else if (r.isImage())
			{
				_ctx.setImageState(r._image, s);
				_ctx.keppAlive(r._image);
			}
			else
			{
				assert(false);
			}
		}
	}

	bool DeviceCommand::updateResources()
	{
		return false;
	}
}