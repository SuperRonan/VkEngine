#include "SynchronizationHelper.hpp"

namespace vkl
{
	//void SynchronizationHelperV1::addSynch(const ResourceInstance& r)
	//{
	//	_resources.push_back(r);
	//}

	//void SynchronizationHelperV1::record()
	//{
	//	for (ResourceInstance& r : _resources)
	//	{
	//		const ResourceState2 next = r.begin_state;
	//		const bool synch_to_readonly = accessIsReadonly2(r.begin_state.access);
	//		if (r.buffers)
	//		{
	//			for (auto& b : r.buffers)
	//			{
	//				if (b.buffer)
	//				{
	//					Buffer::Range range = b.range;
	//					if (range.len == 0)
	//					{
	//						range.len = b.buffer->createInfo().size - range.begin;
	//					}
	//					const DoubleResourceState2 prev = b.buffer->getState(_ctx.resourceThreadId(), range);

	//					bool add_barrier = false;
	//					ResourceState2 synch_from;

	//					if (synch_to_readonly)
	//					{
	//						const bool access_already_synch = (r.begin_state.access & prev.read_only_state.access) == r.begin_state.access;
	//						const bool stage_already_synch = (r.begin_state.stage & prev.read_only_state.stage) == r.begin_state.stage;

	//						if (!access_already_synch || !stage_already_synch)
	//						{
	//							synch_from = prev.write_state;
	//							add_barrier = true;
	//						}
	//					}
	//					else
	//					{
	//						synch_from = prev.read_only_state | prev.write_state;
	//						add_barrier = true;
	//					}
	//					if (add_barrier)
	//					{
	//						VkBufferMemoryBarrier2 barrier = {
	//							.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
	//							.pNext = nullptr,
	//							.srcStageMask = synch_from.stage,
	//							.srcAccessMask = synch_from.access,
	//							.dstStageMask = next.stage,
	//							.dstAccessMask = next.access,
	//							.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	//							.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	//							.buffer = *b.buffer,
	//							.offset = range.begin,
	//							.size = range.len,
	//						};
	//						_buffers_barriers.push_back(barrier);
	//					}
	//					b.buffer->setState(_ctx.resourceThreadId(), range, r.end_state.value_or(r.begin_state));
	//					_ctx.keepAlive(b.buffer);
	//				}
	//			}
	//		}
	//		else if (r.images)
	//		{
	//			for (auto& i : r.images)
	//			{
	//				if (i)
	//				{
	//					const auto prevs = i->getState(_ctx.resourceThreadId());
	//					for (const auto& prev_state_in_range : prevs)
	//					{
	//						bool add_barrier = false;
	//						ResourceState2 synch_from;

	//						const DoubleResourceState2& prev = prev_state_in_range.state;
	//						const VkImageSubresourceRange& range = prev_state_in_range.range;

	//						if (synch_to_readonly)
	//						{
	//							const bool access_already_synch = (r.begin_state.access & prev.read_only_state.access) == r.begin_state.access;
	//							const bool stage_already_synch = (r.begin_state.stage & prev.read_only_state.stage) == r.begin_state.stage;
	//							const bool same_layout = r.begin_state.layout == prev.read_only_state.layout;

	//							if (!access_already_synch || !stage_already_synch || !same_layout)
	//							{
	//								synch_from = prev.write_state;
	//								synch_from.layout = prev.read_only_state.layout;
	//								add_barrier = true;
	//							}
	//						}
	//						else
	//						{
	//							synch_from = prev.read_only_state | prev.write_state;
	//							add_barrier = true;
	//						}

	//						if (add_barrier)
	//						{
	//							VkImageMemoryBarrier2 barrier = {
	//								.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
	//								.pNext = nullptr,
	//								.srcStageMask = synch_from.stage,
	//								.srcAccessMask = synch_from.access,
	//								.dstStageMask = next.stage,
	//								.dstAccessMask = next.access,
	//								.oldLayout = synch_from.layout,
	//								.newLayout = next.layout,
	//								.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	//								.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	//								.image = *i->image(),
	//								.subresourceRange = range,
	//							};
	//							_images_barriers.push_back(barrier);
	//						}
	//					}
	//					i->setState(_ctx.resourceThreadId(), r.end_state.value_or(r.begin_state));
	//					_ctx.keepAlive(i);
	//				}
	//			}
	//		}
	//	}

	//	if (!_images_barriers.empty() || !_buffers_barriers.empty())
	//	{
	//		const VkDependencyInfo dep{
	//			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
	//			.pNext = nullptr,
	//			.dependencyFlags = 0,
	//			.memoryBarrierCount = 0,
	//			.pMemoryBarriers = nullptr,
	//			.bufferMemoryBarrierCount = static_cast<uint32_t>(_buffers_barriers.size()),
	//			.pBufferMemoryBarriers = _buffers_barriers.data(),
	//			.imageMemoryBarrierCount = static_cast<uint32_t>(_images_barriers.size()),
	//			.pImageMemoryBarriers = _images_barriers.data(),
	//		};
	//		vkCmdPipelineBarrier2(*_ctx.getCommandBuffer(), &dep);
	//	}
	//}




	//void ImageSynchronizationHelper::addv(std::shared_ptr<ImageViewInstance> const& ivi, ResourceState2 const& state, std::optional<ResourceState2> const& end_state)
	//{
	//	add(ivi->image(), ivi->createInfo().subresourceRange, state, end_state);
	//}




	//FullyMergedBufferSynchronizationHelper::FullyMergedBufferSynchronizationHelper(ExecutionContext& ctx):
	//	_ctx(ctx)
	//{}

	//void FullyMergedBufferSynchronizationHelper::add(std::shared_ptr<BufferInstance> const& bi, Buffer::Range const& _range, ResourceState2 const& state, std::optional<ResourceState2> const& end_state)
	//{
	//	
	//}

	//std::vector<VkBufferMemoryBarrier2> FullyMergedBufferSynchronizationHelper::commit()
	//{
	//	std::vector<VkBufferMemoryBarrier2> res;
	//	res.reserve(_buffers.size());
	//	for (auto& [bi, bsrs] : _buffers)
	//	{
	//		const DoubleResourceState2 prev = bi->getState(_ctx.resourceThreadId(), bsrs.range);
	//		const bool synch_to_readonly = accessIsReadonly2(bsrs.state.access);
	//		bool add_barrier = false;
	//		ResourceState2 synch_from;

	//		if (synch_to_readonly)
	//		{
	//			const bool access_already_synch = (bsrs.state.access & prev.read_only_state.access) == bsrs.state.access;
	//			const bool stage_already_synch = (bsrs.state.stage & prev.read_only_state.stage) == bsrs.state.stage;
	//			if (!access_already_synch || !stage_already_synch)
	//			{
	//				synch_from = prev.write_state;
	//				add_barrier = true;
	//			}
	//		}
	//		else
	//		{
	//			synch_from = prev.read_only_state | prev.write_state;
	//			add_barrier = true;
	//		}

	//		if (add_barrier)
	//		{
	//			VkBufferMemoryBarrier2 barrier{
	//				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
	//				.pNext = nullptr,
	//				.srcStageMask = synch_from.stage,
	//				.srcAccessMask = synch_from.access,
	//				.dstStageMask = bsrs.state.stage,
	//				.dstAccessMask = bsrs.state.access,
	//				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	//				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	//				.buffer = bi->handle(),
	//				.offset = bsrs.range.begin,
	//				.size = bsrs.range.len,
	//			};
	//			res.push_back(barrier);
	//		}

	//		const ResourceState2 end_state = bsrs.end_state.value_or(bsrs.state);
	//		bi->setState(_ctx.resourceThreadId(), bsrs.range, end_state);
	//		_ctx.keepAlive(bi);
	//	}
	//	return res;
	//}




	//FullyMergedImageSynchronizationHelper::FullyMergedImageSynchronizationHelper(ExecutionContext& ctx):
	//	_ctx(ctx)
	//{}

	//void FullyMergedImageSynchronizationHelper::add(std::shared_ptr<ImageInstance> const& ii, VkImageSubresourceRange const& range, ResourceState2 const& state, std::optional<ResourceState2> const& end_state)
	//{

	//}

	//std::vector<VkImageMemoryBarrier2> FullyMergedImageSynchronizationHelper::commit()
	//{
	//	std::vector<VkImageMemoryBarrier2> res;
	//	static thread_local MyVector<ImageInstance::StateInRange> prevs;
	//	res.reserve(_images.size());
	//	for (auto& [ii, isrs] : _images)
	//	{
	//		const bool synch_to_readonly = accessIsReadonly2(isrs.state.access);
	//		ii->fillState(_ctx.resourceThreadId(), isrs.range, prevs);
	//		for (const auto& prev_state_in_range : prevs)
	//		{
	//			bool add_barrier = false;
	//			ResourceState2 synch_from;
	//			const DoubleResourceState2 & prev = prev_state_in_range.state;
	//			const VkImageSubresourceRange & range = prev_state_in_range.range;
	//			if (synch_to_readonly)
	//			{
	//				const bool access_already_synch = (isrs.state.access & prev.read_only_state.access) == isrs.state.access;
	//				const bool stage_already_synch = (isrs.state.stage & prev.read_only_state.stage) == isrs.state.stage;
	//				const bool same_layout = isrs.state.layout == prev.read_only_state.layout;
	//				if (!(access_already_synch && stage_already_synch && same_layout))
	//				{
	//					synch_from = prev.write_state;
	//					synch_from.layout = prev.read_only_state.layout;
	//					add_barrier = true;
	//				}
	//			}
	//			else
	//			{
	//				synch_from = prev.read_only_state | prev.write_state;
	//				add_barrier = true;
	//			}
	//			if (add_barrier)
	//			{
	//				VkImageMemoryBarrier2 barrier = {
	//					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
	//					.pNext = nullptr,
	//					.srcStageMask = synch_from.stage,
	//					.srcAccessMask = synch_from.access,
	//					.dstStageMask = isrs.state.stage,
	//					.dstAccessMask = isrs.state.access,
	//					.oldLayout = synch_from.layout,
	//					.newLayout = isrs.state.layout,
	//					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	//					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	//					.image = ii->handle(),
	//					.subresourceRange = range,
	//				};
	//				res.push_back(barrier);
	//			}

	//		}
	//		const ResourceState2 end_state = isrs.end_state.value_or(isrs.state);
	//		ii->setState(_ctx.resourceThreadId(), isrs.range, end_state);
	//		_ctx.keepAlive(ii);
	//	}
	//	return res;
	//}



	//bool SynchronizationHelperV2::checkBuffersIntegrity() const
	//{
	//	bool res = true;
	//	for (const auto& [bi, s] : _buffers)
	//	{
	//		bool buffer_ok = true;
	//		for (auto it = s.states.begin(), end = s.states.end(); it != end; ++it)
	//		{
	//			bool range_ok = true;
	//			const BufferNewState::RangeState & rs = *it;
	//			range_ok &= (rs.range.len > 0);
	//			range_ok &= (rs.range.begin < bi->createInfo().size);
	//			range_ok &= (rs.range.end() <= bi->createInfo().size);
	//			if (std::next(it) != end)
	//			{
	//				const BufferNewState::RangeState& nrs = *std::next(it);
	//				range_ok &= (nrs.range.begin >= rs.range.end());
	//			}
	//			buffer_ok &= range_ok;
	//		}
	//		res &= buffer_ok;
	//	}
	//	return res;
	//}

	//bool SynchronizationHelperV2::checkImagesIntegrity() const
	//{
	//	bool res = true;
	//	for (const auto& [ii, s] : _images)
	//	{
	//		bool image_ok = ii->createInfo().mipLevels == s.states.size();
	//		if (image_ok)
	//		{
	//			for (size_t m = 0; m < ii->createInfo().mipLevels; ++m)
	//			{
	//				bool mip_ok = true;
	//				const ImageNewState::MipState & ms = s.states[m];
	//				for (auto it = ms.states.begin(), end = ms.states.end(); it != end; ++it)
	//				{
	//					bool layers_ok = true;
	//					const ImageNewState::LayersNewState & ls = *it;
	//					layers_ok &= (ls.range.len > 0);
	//					layers_ok &= (ls.range.begin < ii->createInfo().mipLevels);
	//					layers_ok &= (ls.range.end() <= ii->createInfo().mipLevels);
	//					if (std::next(it) != end)
	//					{
	//						const ImageNewState::LayersNewState& nls = *std::next(it);
	//						layers_ok &= (nls.range.begin >= ls.range.end());
	//					}
	//					mip_ok &= layers_ok;
	//				}
	//				image_ok &= mip_ok;
	//			}
	//		}
	//		res &= image_ok;
	//	}
	//	return res;
	//}


	//void SynchronizationHelperV2::addSynch(const Resource& r)
	//{
	//	// Left: toward lower values,
	//	// Right: toward higher values,

	//	const bool access_is_read = accessIsRead2(r._begin_state.access);
	//	const bool access_is_write = accessIsWrite2(r._begin_state.access);
	//	const auto access_nature = getAccessNature2(r._begin_state.access);

	//	if (r.isBuffer())
	//	{
	//		std::shared_ptr<BufferInstance> bi = r._buffer->instance();
	//		BufferNewState & bns = _buffers[bi];

	//		// Reduces to the right over time
	//		Buffer::Range range = r._buffer_range.value(); // TODO remove the dyn value in the resource
	//		
	//		auto it = bns.states.begin();
	//		while (it != bns.states.end())
	//		{
	//			Buffer::Range &it_range = it->range;
	//			const bool it_access_is_read = accessIsRead2(it->state.access);
	//			const bool it_access_is_write = accessIsWrite2(it->state.access);
	//			const auto it_access_nature = getAccessNature2(it->state.access);

	//			if (range.begin >= it_range.end()) // Not yet intersect
	//			{
	//				++it;
	//			}
	//			else
	//			{
	//				// Can we merge? (by extending it_range)
	//				const bool can_merge = [&]()
	//				{	
	//					bool res = false;
	//					if (_merge_policy == MergePolicy::SameNature && access_nature == it_access_nature)
	//					{
	//						res = true;
	//					}
	//					else if (_merge_policy >= MergePolicy::AlwaysWhenContiguous)
	//					{
	//						res = true;
	//					}
	//					return res;
	//				}();
	//				if (range.begin < it_range.begin) // range sticks out on the left side with nothing recorded
	//				{

	//					if (can_merge)
	//					{
	//						it_range.len += (it_range.begin - range.begin);
	//						it_range.begin = range.begin;
	//						
	//						it->state |= r._begin_state;
	//						if (it->end_state.has_value())
	//						{
	//							it->end_state.value() |= r._end_state.value_or(r._begin_state);
	//						}

	//						range.begin = it_range.end();

	//					}
	//					else
	//					{
	//						// Fill in the gap
	//						Buffer::Range range_to_insert{
	//							.begin = range.begin,
	//							.len = it_range.begin - range.begin,
	//						};
	//						// Insert left of it_range
	//						it = bns.states.insert(it, BufferNewState::RangeState{
	//							.range = range_to_insert,
	//							.state = r._begin_state,
	//							.end_state = r._end_state,
	//						});
	//						++it;

	//						// Reduce the left part of the range
	//						range.begin = it_range.begin;
	//						range.len -= range_to_insert.len;
	//					}
	//				}

	//				assert(range.begin == it_range.begin);
	//				if (it_range.len == range.len)
	//				{
	//					it->state |= r._begin_state;
	//					if (it->end_state.has_value())
	//					{
	//						it->end_state.value() |= r._end_state.value_or(r._begin_state);
	//					}

	//					range.begin = range.end();
	//					range.len = 0;
	//				}
	//				else if (it_range.len > range.len) // it_range stick out on the right side
	//				{
	//					
	//					range.begin = range.end();
	//					range.len = 0;
	//				}
	//				else // if (it_range.len < range.len)
	//				{

	//				}

	//				if (range.len == 0)
	//				{
	//					break;
	//				}
	//			}
	//		}

	//		if (range.len > 0)
	//		{
	//			bns.states.push_back(BufferNewState::RangeState{
	//				.range = range,
	//				.state = r._begin_state,
	//				.end_state = r._end_state,
	//			});
	//		}
	//	}
	//	else if (r.isImage())
	//	{
	//		std::shared_ptr<ImageInstance> ii = r._image->image()->instance();
	//		ImageNewState & ins = _images[ii];
	//	}
	//}

	//void SynchronizationHelperV2::record()
	//{
	//	std::vector<VkBufferMemoryBarrier2> buffer_barriers;
	//	std::vector<VkImageMemoryBarrier2> image_barriers;

	//	assert(checkBuffersIntegrity());
	//	for (const auto& [bi, s] : _buffers)
	//	{
	//		for(auto it = s.states.begin(), end= s.states.end(); it != end; ++it)
	//		{
	//			const BufferNewState::RangeState & rs = *it;
	//			const ResourceState2 next = rs.state;
	//			const bool synch_to_readonly = accessIsReadonly2(next.access);

	//			const Buffer::Range range = rs.range;
	//			const DoubleResourceState2 prev = bi->getState(_ctx.resourceThreadId(), range);
	//			
	//			bool add_barrier = false;
	//			ResourceState2 synch_from;
	//			if (synch_to_readonly)
	//			{
	//				const bool access_already_synch = (next.access & prev.read_only_state.access) == next.access;
	//				const bool stage_already_synch = (next.stage & prev.read_only_state.stage) == next.stage;
	//				if (!access_already_synch || !stage_already_synch)
	//				{
	//					synch_from = prev.write_state;
	//					add_barrier = true;
	//				}
	//			}
	//			else
	//			{
	//				synch_from = prev.read_only_state | prev.write_state;
	//				add_barrier = true;
	//			}
	//			if (add_barrier)
	//			{
	//				VkBufferMemoryBarrier2 barrier = {
	//					.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
	//					.pNext = nullptr,
	//					.srcStageMask = synch_from.stage,
	//					.srcAccessMask = synch_from.access,
	//					.dstStageMask = next.stage,
	//					.dstAccessMask = next.access,
	//					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	//					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	//					.buffer = bi->handle(),
	//					.offset = range.begin,
	//					.size = range.len,
	//				};
	//				buffer_barriers.push_back(barrier);
	//			}

	//			if (add_barrier)
	//			{
	//				bi->setState(_ctx.resourceThreadId(), range, rs.end_state.value_or(rs.state));
	//			}

	//		}
	//	}

	//	for (const auto& [ii, s] : _images)
	//	{
	//		assert(s.states.size() == ii->createInfo().mipLevels);
	//		for(size_t m = 0; m < ii->createInfo().mipLevels; ++m)
	//		{
	//			const auto & mip_states = s.states[m];
	//			for (size_t i = 0; i < mip_states.states.size(); ++i)
	//			{
	//				const ImageNewState::LayersNewState ls = mip_states.states[i];
	//				const ResourceState2 next = ls.state;
	//				const bool synch_to_readonly = accessIsReadonly2(next.access);

	//				const ImageInstance::Range range{
	//					.aspectMask = s.aspect,
	//					.baseMipLevel = m,
	//					.levelCount = 1,
	//					.baseArrayLayer = ls.range.begin,
	//					.layerCount = ls.range.len,
	//				};
	//				//const DoubleResourceState2 prev = ii->getState(_ctx.resourceThreadId(), range);
	//			}
	//		}
	//	}

	//	if (!buffer_barriers.empty() || !image_barriers.empty())
	//	{
	//		const VkDependencyInfo dep{
	//			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
	//			.pNext = nullptr,
	//			.dependencyFlags = 0,
	//			.memoryBarrierCount = 0,
	//			.pMemoryBarriers = nullptr,
	//			.bufferMemoryBarrierCount = static_cast<uint32_t>(buffer_barriers.size()),
	//			.pBufferMemoryBarriers = buffer_barriers.data(),
	//			.imageMemoryBarrierCount = static_cast<uint32_t>(image_barriers.size()),
	//			.pImageMemoryBarriers = image_barriers.data(),
	//		};
	//		vkCmdPipelineBarrier2(*_ctx.getCommandBuffer(), &dep);
	//	}
	//	
	//}


	//ModularSynchronizationHelper::ModularSynchronizationHelper(ExecutionContext & ctx, BarrierMergePolicy policy):
	//	_ctx(ctx),
	//	_policy(policy),
	//	_fully_merged_buffers(ctx),
	//	_fully_merged_images(ctx)
	//{
	//	assert(_policy == BarrierMergePolicy::Always);
	//}

	//ModularSynchronizationHelper::~ModularSynchronizationHelper()
	//{
	//	if (_policy >= BarrierMergePolicy::AsMuchAsPossible)
	//	{
	//		_fully_merged_buffers.~FullyMergedBufferSynchronizationHelper();
	//	}
	//	if (_policy >= BarrierMergePolicy::Always)
	//	{
	//		_fully_merged_images.~FullyMergedImageSynchronizationHelper();
	//	}
	//}

	//void ModularSynchronizationHelper::addSynch(ResourceInstance const& r)
	//{
	//	if (r.buffers)
	//	{
	//		for (auto& bari : r.buffers)
	//		{
	//			if (bari.buffer)
	//			{
	//				const std::shared_ptr<BufferInstance> & bi = bari.buffer;
	//				Buffer::Range range = bari.range;
	//				assert(!!bi);
	//				if (_policy >= BarrierMergePolicy::AsMuchAsPossible)
	//				{
	//					_fully_merged_buffers.add(bi, range, r.begin_state, r.end_state);
	//				}
	//				else
	//				{
	//					assert(false);
	//				}
	//			}
	//		}
	//	}
	//	else if (r.images)
	//	{
	//		for (auto& ivi : r.images)
	//		{
	//			if (ivi)
	//			{
	//				if (_policy >= BarrierMergePolicy::Always)
	//				{
	//					_fully_merged_images.addv(ivi, r.begin_state, r.end_state);
	//				}
	//				else
	//				{
	//					assert(false);
	//				}
	//			}
	//		}
	//	}
	//	else
	//	{
	//		
	//	}
	//}

	//void ModularSynchronizationHelper::record()
	//{
	//	std::vector<VkBufferMemoryBarrier2> buffer_barriers;
	//	std::vector<VkImageMemoryBarrier2> image_barriers;

	//	if (_policy >= BarrierMergePolicy::AsMuchAsPossible)
	//	{
	//		buffer_barriers = _fully_merged_buffers.commit();
	//	}
	//	else
	//	{
	//		assert(false);
	//	}

	//	if (_policy >= BarrierMergePolicy::Always)
	//	{
	//		image_barriers = _fully_merged_images.commit();
	//	}
	//	else
	//	{
	//		assert(false);
	//	}

	//	if (buffer_barriers.size() + image_barriers.size())
	//	{
	//		const VkDependencyInfo dep{
	//			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
	//			.pNext = nullptr,
	//			.dependencyFlags = 0,
	//			.memoryBarrierCount = 0,
	//			.pMemoryBarriers = nullptr,
	//			.bufferMemoryBarrierCount = static_cast<uint32_t>(buffer_barriers.size()),
	//			.pBufferMemoryBarriers = buffer_barriers.data(),
	//			.imageMemoryBarrierCount = static_cast<uint32_t>(image_barriers.size()),
	//			.pImageMemoryBarriers = image_barriers.data(),
	//		};
	//		vkCmdPipelineBarrier2(*_ctx.getCommandBuffer(), &dep);
	//	}
	//}


	bool InlineSynchronizeBuffer(ExecutionContext& ctx, BufferAndRangeInstance const& bari, ResourceState2 const& begin_state, std::optional<ResourceState2> const& opt_end_state)
	{
		assert(bari.buffer);

		const DoubleResourceState2 prev = bari.buffer->getState(ctx.resourceThreadId(), bari.range);
		const bool synch_to_readonly = accessIsReadonly2(begin_state.access);
		bool add_barrier = false;
		ResourceState2 synch_from;

		if (synch_to_readonly)
		{
			const bool access_already_synch = (begin_state.access & prev.read_only_state.access) == begin_state.access;
			const bool stage_already_synch = (begin_state.stage & prev.read_only_state.stage) == begin_state.stage;
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
			VkBufferMemoryBarrier2 barrier{
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
				.pNext = nullptr,
				.srcStageMask = synch_from.stage,
				.srcAccessMask = synch_from.access,
				.dstStageMask = begin_state.stage,
				.dstAccessMask = begin_state.access,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.buffer = bari.buffer->handle(),
				.offset = bari.range.begin,
				.size = bari.range.len,
			};

			VkDependencyInfo dependecy{
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.pNext = nullptr,
				.dependencyFlags = 0,
				.memoryBarrierCount = 0,
				.pMemoryBarriers = nullptr,
				.bufferMemoryBarrierCount = 1,
				.pBufferMemoryBarriers = &barrier,
				.imageMemoryBarrierCount = 0,
				.pImageMemoryBarriers = nullptr,
			};

			vkCmdPipelineBarrier2(ctx.getCommandBuffer()->handle(), &dependecy);
		}

		const ResourceState2 end_state = opt_end_state.value_or(begin_state);
		bari.buffer->setState(ctx.resourceThreadId(), bari.range, end_state);
		return add_barrier;
	}

	bool InlineSynchronizeImage(ExecutionContext& ctx, std::shared_ptr<ImageInstance> const& ii, VkImageSubresourceRange const& range, ResourceState2 const& begin_state, std::optional<ResourceState2> const& opt_end_state)
	{
		assert(ii);

		static thread_local MyVector<ImageInstance::StateInRange> prevs;
		static thread_local MyVector<VkImageMemoryBarrier2> barriers;

		barriers.clear();

		const bool synch_to_readonly = accessIsReadonly2(begin_state.access);
		ii->fillState(ctx.resourceThreadId(), range, prevs);
		for (const auto& prev_state_in_range : prevs)
		{
			bool add_barrier = false;
			ResourceState2 synch_from;
			const DoubleResourceState2 & prev = prev_state_in_range.state;
			const VkImageSubresourceRange & range = prev_state_in_range.range;
			if (synch_to_readonly)
			{
				const bool access_already_synch = (begin_state.access & prev.read_only_state.access) == begin_state.access;
				const bool stage_already_synch = (begin_state.stage & prev.read_only_state.stage) == begin_state.stage;
				const bool same_layout = begin_state.layout == prev.read_only_state.layout;
				if (!(access_already_synch && stage_already_synch && same_layout))
				{
					synch_from = prev.write_state;
					// If layout transition:
					// Then we need to wait also for the last read only usage
					if (!same_layout)
					{
						synch_from = synch_from | prev.read_only_state;
					}
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
					.dstStageMask = begin_state.stage,
					.dstAccessMask = begin_state.access,
					.oldLayout = synch_from.layout,
					.newLayout = begin_state.layout,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = ii->handle(),
					.subresourceRange = range,
				};
				barriers.push_back(barrier);
			}
		}

		if (barriers)
		{
			VkDependencyInfo dependecy{
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.pNext = nullptr,
				.dependencyFlags = 0,
				.memoryBarrierCount = 0,
				.pMemoryBarriers = nullptr,
				.bufferMemoryBarrierCount = 0,
				.pBufferMemoryBarriers = nullptr,
				.imageMemoryBarrierCount = barriers.size32(),
				.pImageMemoryBarriers = barriers.data(),
			};

			vkCmdPipelineBarrier2(ctx.getCommandBuffer()->handle(), &dependecy);
		}

		const ResourceState2 end_state = opt_end_state.value_or(begin_state);
		ii->setState(ctx.resourceThreadId(), range, end_state);
		return barriers.operator bool();
	}

	bool InlineSynchronizeImageView(ExecutionContext& ctx, std::shared_ptr<ImageViewInstance> const& ivi, ResourceState2 const& begin_state, std::optional<ResourceState2> const& end_state)
	{
		assert(ivi);
		return InlineSynchronizeImage(ctx, ivi->image(), ivi->createInfo().subresourceRange, begin_state, end_state);
	}



	void SynchronizationHelper::reset(ExecutionContext* ctx)
	{
		_ctx = ctx;
		_buffers.clear();
		_images.clear();
	}

	BufferUsageFunction SynchronizationHelper::getBufferProcessFunction()
	{
		return [this](std::shared_ptr<BufferInstance> const& bi, const BufferSubRangeState * states, size_t count)
		{
			for (size_t i = 0; i < count; ++i)
			{
				const BufferSubRangeState & state = states[i];
				const ResourceState2 & begin_state = state.state;
				const DoubleResourceState2 prev = bi->getState(_ctx->resourceThreadId(), state.range);
				const bool synch_to_readonly = accessIsReadonly2(begin_state.access);
				bool add_barrier = false;
				ResourceState2 synch_from;

				if (synch_to_readonly)
				{
					const bool access_already_synch = (begin_state.access & prev.read_only_state.access) == begin_state.access;
					const bool stage_already_synch = (begin_state.stage & prev.read_only_state.stage) == begin_state.stage;
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
					VkBufferMemoryBarrier2 barrier{
						.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
						.pNext = nullptr,
						.srcStageMask = synch_from.stage,
						.srcAccessMask = synch_from.access,
						.dstStageMask = begin_state.stage,
						.dstAccessMask = begin_state.access,
						.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.buffer = bi->handle(),
						.offset = state.range.begin,
						.size = state.range.len,
					};
					_buffers.push_back(barrier);
				}
			}

			for (size_t i = 0; i < count; ++i)
			{
				const BufferSubRangeState& state = states[i];
				const ResourceState2 end_state = state.end_state.value_or(state.state);
				bi->setState(_ctx->resourceThreadId(), state.range, end_state);
			}
			_ctx->keepAlive(bi);
		};
	}

	ImageUsageFunction SynchronizationHelper::getImageProcessFunction()
	{
		static thread_local MyVector<ImageInstance::StateInRange> prevs;
		return [this](std::shared_ptr<ImageInstance> const& ii, const ImageSubRangeState* states, size_t count)
		{
			for (size_t i = 0; i < count; ++i)
			{
				prevs.clear();
				const ImageSubRangeState & state = states[i];
				const ResourceState2 & begin_state = state.state;
				ii->fillState(_ctx->resourceThreadId(), state.range, prevs);
				const bool synch_to_readonly = accessIsReadonly2(state.state.access);
				for (const auto& prev_state_in_range : prevs)
				{
					bool add_barrier = false;
					ResourceState2 synch_from;
					const DoubleResourceState2& prev = prev_state_in_range.state;
					const VkImageSubresourceRange& range = prev_state_in_range.range;
					if (synch_to_readonly)
					{
						const bool access_already_synch = (begin_state.access & prev.read_only_state.access) == begin_state.access;
						const bool stage_already_synch = (begin_state.stage & prev.read_only_state.stage) == begin_state.stage;
						const bool same_layout = begin_state.layout == prev.read_only_state.layout;
						if (!(access_already_synch && stage_already_synch && same_layout))
						{
							synch_from = prev.write_state;
							// If layout transition:
							// Then we need to wait also for the last read only usage
							if (!same_layout)
							{
								synch_from = synch_from | prev.read_only_state;
							}
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
							.dstStageMask = begin_state.stage,
							.dstAccessMask = begin_state.access,
							.oldLayout = synch_from.layout,
							.newLayout = begin_state.layout,
							.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
							.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
							.image = ii->handle(),
							.subresourceRange = range,
						};
						_images.push_back(barrier);
					}
				}
			}

			for (size_t i = 0; i < count; ++i)
			{
				const ImageSubRangeState& state = states[i];
				const ResourceState2 end_state = state.end_state.value_or(state.state);
				ii->setState(_ctx->resourceThreadId(), state.range, end_state);
			}
			_ctx->keepAlive(ii);
		};
	}

	void SynchronizationHelper::commit(AbstractBufferUsageList const& buffers)
	{
		buffers.iterate(getBufferProcessFunction());
	}

	void SynchronizationHelper::commit(AbstractImageUsageList const& images)
	{
		images.iterate(getImageProcessFunction());
	}
	
	void SynchronizationHelper::commit(ResourceUsageList const& resource_list)
	{
		resource_list.iterateOnBuffers(getBufferProcessFunction());
		resource_list.iterateOnImages(getImageProcessFunction());
	}

	void SynchronizationHelper::record()
	{
		if (_buffers || _images)
		{
			assert(_ctx);
			VkDependencyInfo dependecy{
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.pNext = nullptr,
				.dependencyFlags = 0,
				.memoryBarrierCount = 0,
				.pMemoryBarriers = nullptr,
				.bufferMemoryBarrierCount = _buffers.size32(),
				.pBufferMemoryBarriers = _buffers.data(),
				.imageMemoryBarrierCount = _images.size32(),
				.pImageMemoryBarriers = _images.data(),
			};

			vkCmdPipelineBarrier2(_ctx->getCommandBuffer()->handle(), &dependecy);
		}
	}


} // namespace vkl
