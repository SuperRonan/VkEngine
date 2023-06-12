#include "TransferCommand.hpp"

namespace vkl
{
	BlitImage::BlitImage(CreateInfo const& ci) :
		TransferCommand(ci.app, ci.name),
		_src(ci.src),
		_dst(ci.dst),
		_regions(ci.regions),
		_filter(ci.filter)
	{}

	void BlitImage::execute(ExecutionContext& context, BlitInfo const& bi)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		InputSynchronizationHelper synch(context);
		
		synch.addSynch(Resource{
			._image = bi.src,
			._begin_state = ResourceState2{
				._access = VK_ACCESS_2_TRANSFER_READ_BIT,
				._layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				._stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT
			},
			._image_usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		});

		synch.addSynch(Resource{
			._image = bi.dst,
			._begin_state = ResourceState2{
				._access = VK_ACCESS_2_TRANSFER_READ_BIT,
				._layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				._stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT
			},
			._image_usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		});

		synch.record();

		const VkImageBlit * regions = bi.regions.data();
		uint32_t n_regions = static_cast<uint32_t>(bi.regions.size());
		VkImageBlit _region;
		if (bi.regions.empty())
		{
			_region = VkImageBlit{
				.srcSubresource = getImageLayersFromRange(bi.src->range()),
				.srcOffsets = {makeZeroOffset3D(), convert(*bi.src->image()->extent())},
				.dstSubresource = getImageLayersFromRange(bi.dst->range()),
				.dstOffsets = {makeZeroOffset3D(), convert(*bi.dst->image()->extent())},
			};
			regions = &_region;
			n_regions = 1;
		}
		

		vkCmdBlitImage(*cmd,
			*bi.src->image()->instance(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			*bi.dst->image()->instance(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			n_regions, regions, bi.filter
		);

		synch.NotifyContext();
	}

	void BlitImage::execute(ExecutionContext& context)
	{
		BlitInfo bi{
			.src = _src,
			.dst = _dst,
			.regions = _regions,
			.filter = _filter,
		};
		execute(context, bi);
	}

	Executable BlitImage::with(BlitInfo const& bi)
	{
		return [&](ExecutionContext& ctx)
		{
			BlitInfo _bi{
				.src = bi.src ? bi.src : _src,
				.dst = bi.dst ? bi.dst : _dst,
				.regions = bi.regions.empty() ? _regions : bi.regions,
				.filter = bi.filter == VK_FILTER_MAX_ENUM ? _filter : bi.filter,
			};
			this->execute(ctx, _bi);
		};
	}

	CopyImage::CopyImage(CreateInfo const& ci):
		TransferCommand(ci.app, ci.name),
		_src(ci.src),
		_dst(ci.dst),
		_regions(ci.regions)
	{}

	void CopyImage::init()
	{

	}

	void CopyImage::execute(ExecutionContext& ctx, CopyInfo const& cinfo)
	{
		std::shared_ptr<CommandBuffer> cmd = ctx.getCommandBuffer();
		InputSynchronizationHelper synch(ctx);

		synch.addSynch(Resource{
			._image = cinfo.src,
			._begin_state = ResourceState2{
				._access = VK_ACCESS_2_TRANSFER_READ_BIT,
				._layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				._stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT
			},
			._image_usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			});

		synch.addSynch(Resource{
			._image = cinfo.dst,
			._begin_state = ResourceState2{
				._access = VK_ACCESS_2_TRANSFER_READ_BIT,
				._layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				._stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT
			},
			._image_usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			});

		synch.record();

		const VkImageCopy* regions = cinfo.regions.data();
		uint32_t n_regions = static_cast<uint32_t>(cinfo.regions.size());
		VkImageCopy _region;
		if (cinfo.regions.empty())
		{
			_region = VkImageCopy{
				.srcSubresource = getImageLayersFromRange(cinfo.src->range()),
				.srcOffset = makeZeroOffset3D(),
				.dstSubresource = getImageLayersFromRange(cinfo.dst->range()),
				.dstOffset = makeZeroOffset3D(),
				.extent = *cinfo.dst->image()->extent(),
			};
			regions = &_region;
			n_regions = 1;
		}


		vkCmdCopyImage(*cmd,
			*cinfo.src->image()->instance(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			*cinfo.dst->image()->instance(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			n_regions, regions
		);

		synch.NotifyContext();
	}

	void CopyImage::execute(ExecutionContext& context)
	{
		CopyInfo cinfo{
			.src = _src,
			.dst = _dst,
			.regions = _regions,
		};
		execute(context, cinfo);
	}

	Executable CopyImage::with(CopyInfo const& ci)
	{
		return [&](ExecutionContext& ctx)
		{
			CopyInfo cinfo{
				.src = ci.src ? ci.src : _src,
				.dst = ci.dst ? ci.dst : _dst,
				.regions = ci.regions.empty() ? _regions : ci.regions,
			};
			execute(ctx, cinfo);
		};
	}


	CopyBufferToImage::CopyBufferToImage(CreateInfo const& ci) :
		TransferCommand(ci.app, ci.name),
		_src(ci.src),
		_dst(ci.dst),
		_regions(ci.regions)
	{}

	void CopyBufferToImage::init()
	{
	}

	void CopyBufferToImage::execute(ExecutionContext& context, CopyInfo const& cinfo)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		InputSynchronizationHelper synch(context);
		synch.addSynch(Resource{
			._buffer = cinfo.src,
			._begin_state = ResourceState2{
				._access = VK_ACCESS_2_TRANSFER_READ_BIT,
				._stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT
			},
			._buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		});
		synch.addSynch(Resource{
			._image = cinfo.dst,
			._begin_state = ResourceState2{
				._access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				._layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				._stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			},
			._image_usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		});

		synch.record();


		uint32_t n_regions = static_cast<uint32_t>(cinfo.regions.size());
		const VkBufferImageCopy* p_regions = cinfo.regions.data();
		VkBufferImageCopy _reg;
		if (n_regions == 0)
		{
			const VkExtent3D extent = *cinfo.dst->image()->extent();
			_reg = VkBufferImageCopy{
				.bufferOffset = 0,
				.bufferRowLength = extent.height,
				.bufferImageHeight = extent.height,
				.imageSubresource = getImageLayersFromRange(cinfo.dst->range()),
				.imageOffset = makeZeroOffset3D(),
				.imageExtent = extent,
			};
			p_regions = &_reg;
			n_regions = 1;
		}

		vkCmdCopyBufferToImage(
			*cmd,
			*cinfo.src->instance(), *cinfo.dst->image()->instance(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			n_regions, p_regions
		);


		synch.NotifyContext();
	}

	void CopyBufferToImage::execute(ExecutionContext& context)
	{
		const CopyInfo cinfo{
			.src = _src,
			.dst = _dst,
			.regions = _regions,
		};
		execute(context, cinfo);
	}

	Executable CopyBufferToImage::with(CopyInfo const& info)
	{
		return [&](ExecutionContext& context)
		{
			const CopyInfo cinfo{
				.src = info.src ? info.src : _src,
				.dst = info.dst ? info.dst : _dst,
				.regions = info.regions.empty() ? _regions : info.regions,
			};
			return execute(context, cinfo);
		};
	}


	CopyBuffer::CopyBuffer(CreateInfo const& ci) :
		TransferCommand(ci.app, ci.name),
		_src(ci.src),
		_dst(ci.dst)
	{}

	void CopyBuffer::init()
	{
	}

	void CopyBuffer::execute(ExecutionContext& context, CopyInfo const& cinfo)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		InputSynchronizationHelper synch(context);
		synch.addSynch(Resource{
			._buffer = cinfo.src,
			._begin_state = ResourceState2{
				._access = VK_ACCESS_2_TRANSFER_READ_BIT,
				._stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			},
			._buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		});
		synch.addSynch(Resource{
			._buffer = cinfo.dst,
			._begin_state = ResourceState2{
				._access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				._stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			},
			._buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		});
		
		synch.record();

		// TODO from the ci
		VkBufferCopy region = {
			.srcOffset = 0,
			.dstOffset = 0,
			.size = std::min(cinfo.src->instance()->createInfo().size, cinfo.dst->instance()->createInfo().size),
		};

		vkCmdCopyBuffer(*cmd,
			*cinfo.src->instance(), *cinfo.dst->instance(),
			1, &region
		);

		synch.NotifyContext();
	}

	void CopyBuffer::execute(ExecutionContext& context)
	{
		const CopyInfo cinfo{
			.src = _src,
			.dst = _dst,
		};
		execute(context, cinfo);
	}

	Executable CopyBuffer::with(CopyInfo const& cinfo)
	{
		return [&](ExecutionContext& context)
		{
			const CopyInfo ci{
				.src = cinfo.src ? cinfo.src : _src,
				.dst = cinfo.dst ? cinfo.dst : _dst,
			};
			execute(context, ci);
		};
	}


	FillBuffer::FillBuffer(CreateInfo const& ci) :
		TransferCommand(ci.app, ci.name),
		_buffer(ci.buffer),
		_begin(ci.begin),
		_size(ci.size),
		_value(ci.value)
	{

	}

	void FillBuffer::init()
	{

	}

	void FillBuffer::execute(ExecutionContext& context, FillInfo const& fi)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		InputSynchronizationHelper synch(context);
		synch.addSynch(Resource{
			._buffer = fi.buffer,
			._begin_state = ResourceState2{
				._access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				._stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			},
			._buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		});
		
		synch.record();

		vkCmdFillBuffer(*cmd, *fi.buffer->instance(), fi.begin.value(), fi.size.value(), fi.value.value());

		synch.NotifyContext();
	}

	void FillBuffer::execute(ExecutionContext& context)
	{
		const FillInfo fi{
			.buffer = _buffer,
			.begin = _begin, 
			.size = _size,
			.value = _value,
		};
		execute(context, fi);
	}

	Executable FillBuffer::with(FillInfo const& fi)
	{
		return [&](ExecutionContext& context)
		{
			const FillInfo _fi{
				.buffer = fi.buffer ? fi.buffer : _buffer,
				.begin = fi.begin.value_or(_begin),
				.size = fi.size.value_or(_size),
				.value = fi.value.value_or(_value),
			};
			execute(context, _fi);
		};
	}



	ClearImage::ClearImage(CreateInfo const& ci):
		TransferCommand(ci.app, ci.name),
		_view(ci.view),
		_value(ci.value)
	{

	}

	void ClearImage::init()
	{

	}

	void ClearImage::execute(ExecutionContext& context, ClearInfo const& ci)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		InputSynchronizationHelper synch(context);
		synch.addSynch(Resource{
			._image = ci.view,
			._begin_state = ResourceState2{
				._access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				._layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				._stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			},
			._image_usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		});

		synch.record();

		const VkImageSubresourceRange range = ci.view->range();

		const VkClearValue value = ci.value.value();

		// TODO depth clear if applicable
		vkCmdClearColorImage(
			*cmd, 
			*ci.view->image()->instance(), 
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
			&value.color, 
			1, 
			&range
		);

		synch.NotifyContext();
	}

	void ClearImage::execute(ExecutionContext& context)
	{
		const ClearInfo ci{
			.view = _view,
			.value = _value,
		};
		execute(context, ci);
	}

	Executable ClearImage::with(ClearInfo const& ci)
	{
		return [&](ExecutionContext& context)
		{
			const ClearInfo cinfo{
				.view = ci.view ? ci.view : _view,
				.value = ci.value.value_or(_value),
			};
			execute(context, cinfo);
		};
	}

	UpdateBuffer::UpdateBuffer(CreateInfo const& ci) :
		TransferCommand(ci.app, ci.name),
		_src(ci.src),
		_dst(ci.dst)
	{}

	UpdateBuffer::~UpdateBuffer()
	{

	}

	void UpdateBuffer::execute(ExecutionContext& ctx, UpdateInfo const& ui)
	{
		InputSynchronizationHelper synch(ctx);
		synch.addSynch(Resource{
			._buffer = ui.dst,
			._begin_state = ResourceState2{
				._access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				._stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			},
			._buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		});
		synch.record();
		vkCmdUpdateBuffer(*ctx.getCommandBuffer(), *ui.dst->instance(), ui.offset, ui.src.size(), ui.src.data());
		synch.NotifyContext();
	}

	void UpdateBuffer::execute(ExecutionContext& ctx)
	{
		UpdateInfo ui{
			.src = _src,
			.dst = _dst,
			.offset = 0,
		};
		execute(ctx, ui);
	}

	Executable UpdateBuffer::with(UpdateInfo const& ui)
	{
		UpdateInfo _ui
		{
			.src = ui.src.hasValue() ? ui.src : _src,
			.dst = ui.dst ? ui.dst : _dst,
			.offset = ui.offset,
		};
		return [=](ExecutionContext& ctx)
		{
			execute(ctx, _ui);
		};
	}
	


	void ComputeMips::execute(ExecutionContext& ctx, ExecInfo const& ei)
	{
		InputSynchronizationHelper synch(ctx);
		CommandBuffer& cmd = *ctx.getCommandBuffer();

		ImageViewInstance & view = *ei.target->instance();
		ImageInstance & img = *ei.target->image()->instance();

		synch.addSynch(Resource{
			._image = ei.target,
			._begin_state = {
				._access = VK_ACCESS_2_TRANSFER_READ_BIT,
				._layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				._stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
			},
			._end_state = ResourceState2{
				._access = VK_ACCESS_2_TRANSFER_READ_BIT,
				._layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				._stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			},
			._image_usage = VK_IMAGE_USAGE_TRANSFER_BITS,
		});

		synch.record();


		const uint32_t m = img.createInfo().mipLevels;
		VkExtent3D extent = img.createInfo().extent;

		for (uint32_t i = 1; i < m; ++i)
		{
			std::array<VkImageMemoryBarrier2, 2> barriers;
			barriers[0] = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.pNext = nullptr,
				.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
				.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = img,
				.subresourceRange = {
					.aspectMask = view.createInfo().subresourceRange.aspectMask,
					.baseMipLevel = i,
					.levelCount = 1,
					.baseArrayLayer = view.createInfo().subresourceRange.baseArrayLayer,
					.layerCount = view.createInfo().subresourceRange.layerCount,
				},
			};
			uint32_t num_barriers = 1;
			if (i > 1)
			{
				num_barriers = 2;
				barriers[1] = {
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
					.pNext = nullptr,
					.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
					.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
					.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = img,
					.subresourceRange = {
						.aspectMask = view.createInfo().subresourceRange.aspectMask,
						.baseMipLevel = i - 1,
						.levelCount = 1,
						.baseArrayLayer = view.createInfo().subresourceRange.baseArrayLayer,
						.layerCount = view.createInfo().subresourceRange.layerCount,
					},
				};
			}
			VkDependencyInfo dep = {
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.pNext = nullptr,
				.dependencyFlags = 0,
				.memoryBarrierCount = 0,
				.pMemoryBarriers = nullptr,
				.bufferMemoryBarrierCount = 0,
				.pBufferMemoryBarriers = nullptr,
				.imageMemoryBarrierCount = num_barriers,
				.pImageMemoryBarriers = barriers.data(),
			};

			vkCmdPipelineBarrier2(cmd, &dep);

			VkExtent3D smaller_extent = {
				.width = std::max(1u, extent.width / 2),
				.height = std::max(1u, extent.height / 2),
				.depth = std::max(1u, extent.depth / 2),
			};

			VkImageBlit2 region = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
				.pNext = nullptr,
				.srcSubresource = {
					.aspectMask = view.createInfo().subresourceRange.aspectMask,
					.mipLevel = i - 1,
					.baseArrayLayer = view.createInfo().subresourceRange.baseArrayLayer,
					.layerCount = view.createInfo().subresourceRange.layerCount,
				},
				.srcOffsets = {
					makeZeroOffset3D(), convert(extent),
				},
				.dstSubresource = {
					.aspectMask = view.createInfo().subresourceRange.aspectMask,
					.mipLevel = i,
					.baseArrayLayer = view.createInfo().subresourceRange.baseArrayLayer,
					.layerCount = view.createInfo().subresourceRange.layerCount,
				},
				.dstOffsets = {
					makeZeroOffset3D(), convert(smaller_extent),
				},
			};

			VkBlitImageInfo2 blit{
				.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
				.pNext = nullptr,
				.srcImage = img,
				.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				.dstImage = img,
				.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.regionCount = 1,
				.pRegions = &region,
				.filter = VK_FILTER_LINEAR,
			};

			vkCmdBlitImage2(cmd, &blit);

			extent = smaller_extent;
		}

		// One last barrier
		{
			VkImageMemoryBarrier2 barrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.pNext = nullptr,
				.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = img,
				.subresourceRange = {
					.aspectMask = view.createInfo().subresourceRange.aspectMask,
					.baseMipLevel = m - 1,
					.levelCount = 1,
					.baseArrayLayer = view.createInfo().subresourceRange.baseArrayLayer,
					.layerCount = view.createInfo().subresourceRange.layerCount,
				},
			};

			VkDependencyInfo dep = {
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.pNext = nullptr,
				.dependencyFlags = 0,
				.memoryBarrierCount = 0,
				.pMemoryBarriers = nullptr,
				.bufferMemoryBarrierCount = 0,
				.pBufferMemoryBarriers = nullptr,
				.imageMemoryBarrierCount = 1,
				.pImageMemoryBarriers = &barrier,
			};

			vkCmdPipelineBarrier2(cmd, &dep);
		}


		synch.NotifyContext();

	}

	void ComputeMips::execute(ExecutionContext & ctx)
	{
		ExecInfo ei{
			.target = _target,
		};
		execute(ctx, ei);
	}

	Executable ComputeMips::with(ExecInfo const& ei)
	{
		ExecInfo _ei{
			.target = ei.target ? ei.target : _target,
		};
		return [=](ExecutionContext& ctx)
		{
			execute(ctx, _ei);
		};
	}



	UploadBuffer::UploadBuffer(CreateInfo const& ci):
		TransferCommand(ci.app, ci.name),
		_src(ci.src),
		_dst(ci.dst),
		_use_update_buffer_ifp(ci.use_update_buffer_ifp)
	{}

	void UploadBuffer::execute(ExecutionContext& ctx, UploadInfo const& ui)
	{
		const bool use_update = [&](){
			bool res = ui.use_update_buffer_ifp.value();
			if (res)
			{
				const uint32_t max_size = 65536;
				res &= (ui.src.size() <= max_size);
				res &= (ui.src.size() % 4 == 0);
			}
			return true;
		}();

		if (use_update)
		{

		}
		else // Use Staging Buffer
		{

		}
	}
}