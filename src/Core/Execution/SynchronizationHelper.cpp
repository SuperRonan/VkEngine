#include "SynchronizationHelper.hpp"

namespace vkl
{
	void SynchronizationHelperV1::addSynch(const Resource& r)
	{
		
		const ResourceState2 next = r._begin_state;
		const bool synch_to_readonly = accessIsReadonly2(r._begin_state.access);
		if (r.isImage())
		{
			_resources.push_back(r);
			assert(r._image->instance());
			const auto prevs = r._image->instance()->getState(_ctx.resourceThreadId());
			for (const auto& prev_state_in_range : prevs)
			{
				bool add_barrier = false;
				ResourceState2 synch_from;

				const DoubleResourceState2& prev = prev_state_in_range.state;
				const VkImageSubresourceRange& range = prev_state_in_range.range;

				if (synch_to_readonly)
				{
					const bool access_already_synch = (r._begin_state.access & prev.read_only_state.access) == r._begin_state.access;
					const bool stage_already_synch = (r._begin_state.stage & prev.read_only_state.stage) == r._begin_state.stage;
					const bool same_layout = r._begin_state.layout == prev.read_only_state.layout;

					if (!access_already_synch || !stage_already_synch || !same_layout)
					{
						synch_from = prev.write_state;
						synch_from.layout = prev.read_only_state.layout;
						add_barrier = true;
					}
				}
				else
				{
					synch_from = prev.read_only_state | prev.write_state;
					add_barrier = true;
				}

				if (add_barrier)
				{
					VkImageMemoryBarrier2 barrier = {
						.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
						.pNext = nullptr,
						.srcStageMask = synch_from.stage,
						.srcAccessMask = synch_from.access,
						.dstStageMask = next.stage,
						.dstAccessMask = next.access,
						.oldLayout = synch_from.layout,
						.newLayout = next.layout,
						.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.image = *r._image->image()->instance(),
						.subresourceRange = range,
					};
					_images_barriers.push_back(barrier);
				}
			}
		}
		else if (r.isBuffer())
		{
			if (r._buffer->name() == ".mesh_buffer"s)
			{
				int _ = 0;
			}
			_resources.push_back(r);
			assert(r._buffer->instance());
			Buffer::Range range = r._buffer_range.value();
			if (range.len == 0)
			{
				range.len = r._buffer->instance()->createInfo().size - range.begin;
			}
			const DoubleResourceState2 prev = r._buffer->instance()->getState(_ctx.resourceThreadId(), range);
			
			bool add_barrier = false;
			ResourceState2 synch_from;

			if (synch_to_readonly)
			{
				const bool access_already_synch = (r._begin_state.access & prev.read_only_state.access) == r._begin_state.access;
				const bool stage_already_synch = (r._begin_state.stage & prev.read_only_state.stage) == r._begin_state.stage;

				if (!access_already_synch || !stage_already_synch)
				{
					synch_from = prev.write_state;
					add_barrier = true;
				}
			}
			else
			{
				synch_from = prev.read_only_state | prev.write_state;
				add_barrier = true;
			}
			if (add_barrier)
			{
				VkBufferMemoryBarrier2 barrier = {
					.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
					.pNext = nullptr,
					.srcStageMask = synch_from.stage,
					.srcAccessMask = synch_from.access,
					.dstStageMask = next.stage,
					.dstAccessMask = next.access,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.buffer = *r._buffer->instance(),
					.offset = range.begin,
					.size = range.len,
				};
				_buffers_barriers.push_back(barrier);
			}
		}
		else
		{
			
		}
	}

	void SynchronizationHelperV1::record()
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
		
		for (const auto& r : _resources)
		{
			ResourceState2 const& s = r._end_state.value_or(r._begin_state);
			if (r.isBuffer())
			{
				if (r._buffer->name() == ".mesh_buffer"s)
				{
					int _ = 0;
				}
				r._buffer->instance()->setState(_ctx.resourceThreadId(), r._buffer_range.value(), s);
				_ctx.keppAlive(r._buffer->instance());
			}
			else if (r.isImage())
			{
				r._image->instance()->setState(_ctx.resourceThreadId(), s);
				_ctx.keppAlive(r._image->instance());
			}
			else
			{
				assert(false);
			}
		}
	}

	void SynchronizationHelperV2::addSynch(const Resource& r)
	{

	}

	void SynchronizationHelperV2::record()
	{
		
	}
} // namespace vkl
