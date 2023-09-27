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
		SynchronizationHelper synch(context);
		
		synch.addSynch(Resource{
			._image = bi.src,
			._begin_state = ResourceState2{
				.access = VK_ACCESS_2_TRANSFER_READ_BIT,
				.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT
			},
			._image_usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		});

		synch.addSynch(Resource{
			._image = bi.dst,
			._begin_state = ResourceState2{
				.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT
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
		SynchronizationHelper synch(ctx);

		synch.addSynch(Resource{
			._image = cinfo.src,
			._begin_state = ResourceState2{
				.access = VK_ACCESS_2_TRANSFER_READ_BIT,
				.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT
			},
			._image_usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			});

		synch.addSynch(Resource{
			._image = cinfo.dst,
			._begin_state = ResourceState2{
				.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT
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
		_range(ci.range),
		_dst(ci.dst),
		_regions(ci.regions)
	{}

	void CopyBufferToImage::init()
	{
	}

	void CopyBufferToImage::execute(ExecutionContext& context, CopyInfo const& cinfo)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		SynchronizationHelper synch(context);
		synch.addSynch(Resource{
			._buffer = cinfo.src,
			._buffer_range = cinfo.range,
			._begin_state = ResourceState2{
				.access = VK_ACCESS_2_TRANSFER_READ_BIT,
				.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT
			},
			._buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		});
		synch.addSynch(Resource{
			._image = cinfo.dst,
			._begin_state = ResourceState2{
				.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
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
				.bufferRowLength = 0,   // 0 => tightly packed
				.bufferImageHeight = 0, // 0 => tightly packed
				.imageSubresource = getImageLayersFromRange(cinfo.dst->instance()->createInfo().subresourceRange),
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
	}

	void CopyBufferToImage::execute(ExecutionContext& context)
	{
		const CopyInfo cinfo{
			.src = _src,
			.range = _range.value(),
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
				.range = info.range.len ? info.range : _range.value(),
				.dst = info.dst ? info.dst : _dst,
				.regions = info.regions.empty() ? _regions : info.regions,
			};
			return execute(context, cinfo);
		};
	}

	CopyBuffer::CopyBuffer(CreateInfo const& ci) :
		TransferCommand(ci.app, ci.name),
		_src(ci.src),
		_src_offset(ci.src_offset),
		_dst(ci.dst),
		_dst_offset(ci.dst_offset),
		_size(ci.size)
	{}

	void CopyBuffer::init()
	{
	}

	void CopyBuffer::execute(ExecutionContext& context, CopyInfo const& cinfo)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		SynchronizationHelper synch(context);
		synch.addSynch(Resource{
			._buffer = cinfo.src,
			._buffer_range = Range_st{.begin = cinfo.src_offset, .len = cinfo.size},
			._begin_state = ResourceState2{
				.access = VK_ACCESS_2_TRANSFER_READ_BIT,
				.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			},
			._buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		});
		synch.addSynch(Resource{
			._buffer = cinfo.dst,
			._buffer_range = Range_st{.begin = cinfo.dst_offset, .len = cinfo.size},
			._begin_state = ResourceState2{
				.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			},
			._buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		});
		
		synch.record();

		size_t size = cinfo.size; 
		VkBufferCopy region = {
			.srcOffset = cinfo.src_offset,
			.dstOffset = cinfo.dst_offset,
			.size = size,
		};

		vkCmdCopyBuffer(*cmd,
			*cinfo.src->instance(), *cinfo.dst->instance(),
			1, &region
		);
	}

	void CopyBuffer::execute(ExecutionContext& context)
	{
		CopyInfo cinfo{
			.src = _src,
			.src_offset = _src_offset.value(),
			.dst = _dst,
			.dst_offset = _dst_offset.value(),
			.size = _size.value(),
		};
		if (cinfo.size == 0)
		{
			size_t src_max_size = _src->instance()->createInfo().size - cinfo.src_offset;
			size_t dst_max_size = _dst->instance()->createInfo().size - cinfo.dst_offset;
			cinfo.size = std::min(src_max_size, dst_max_size);
		}
		execute(context, cinfo);
	}

	Executable CopyBuffer::with(CopyInfo const& cinfo)
	{
		CopyInfo ci{
			.src = cinfo.src ? cinfo.src : _src,
			.src_offset = cinfo.src_offset ? cinfo.src_offset : _src_offset.value(),
			.dst = cinfo.dst ? cinfo.dst : _dst,
			.dst_offset = cinfo.dst_offset ? cinfo.dst_offset : _dst_offset.value(),
			.size = cinfo.size ? cinfo.size : _size.value(),
		};
		if (cinfo.size == 0)
		{
			size_t src_max_size = ci.src->instance()->createInfo().size - cinfo.src_offset;
			size_t dst_max_size = ci.dst->instance()->createInfo().size - cinfo.dst_offset;
			ci.size = std::min(src_max_size, dst_max_size);
		}
		return [this, ci](ExecutionContext& context)
		{
			execute(context, ci);
		};
	}


	FillBuffer::FillBuffer(CreateInfo const& ci) :
		TransferCommand(ci.app, ci.name),
		_buffer(ci.buffer),
		_range(ci.range),
		_value(ci.value)
	{

	}

	void FillBuffer::init()
	{

	}

	void FillBuffer::execute(ExecutionContext& context, FillInfo const& fi)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		SynchronizationHelper synch(context);
		synch.addSynch(Resource{
			._buffer = fi.buffer,
			._buffer_range = fi.range.value(),
			._begin_state = ResourceState2{
				.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			},
			._buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		});
		
		synch.record();
		VkDeviceSize len = fi.range.value().len;
		if (len == 0)
		{
			len = fi.buffer->instance()->createInfo().size;
		}
		vkCmdFillBuffer(*cmd, *fi.buffer->instance(), fi.range.value().begin, len, fi.value.value());
	}

	void FillBuffer::execute(ExecutionContext& context)
	{
		const FillInfo fi{
			.buffer = _buffer,
			.range = _range.value(),
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
				.range = fi.range.has_value() ? fi.range.value() : _range.value(),
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
		SynchronizationHelper synch(context);
		synch.addSynch(Resource{
			._image = ci.view,
			._begin_state = ResourceState2{
				.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
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
		_dst(ci.dst),
		_offset(ci.offset)
	{}

	UpdateBuffer::~UpdateBuffer()
	{

	}

	void UpdateBuffer::execute(ExecutionContext& ctx, UpdateInfo const& ui)
	{
		const bool buffer_ok = ui.dst->instance().operator bool();
		assert(buffer_ok);
		SynchronizationHelper synch(ctx);
		synch.addSynch(Resource{
			._buffer = ui.dst,
			._buffer_range = Range_st{.begin = ui.offset.value(), .len = ui.src.size(), },
			._begin_state = ResourceState2{
				.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			},
			._buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		});
		synch.record();
		vkCmdUpdateBuffer(*ctx.getCommandBuffer(), *ui.dst->instance(), ui.offset.value(), ui.src.size(), ui.src.data());
	}

	void UpdateBuffer::execute(ExecutionContext& ctx)
	{
		UpdateInfo ui{
			.src = _src,
			.dst = _dst,
			.offset = _offset.value(),
		};
		execute(ctx, ui);
	}

	Executable UpdateBuffer::with(UpdateInfo const& ui)
	{
		UpdateInfo _ui
		{
			.src = ui.src.hasValue() ? ui.src : _src,
			.dst = ui.dst ? ui.dst : _dst,
			.offset = ui.offset.has_value() ? ui.offset.value() : _offset.value(),
		};
		return [=](ExecutionContext& ctx)
		{
			execute(ctx, _ui);
		};
	}
	


	void ComputeMips::execute(ExecutionContext& ctx, ExecInfo const& ei)
	{
		SynchronizationHelper synch(ctx);
		CommandBuffer& cmd = *ctx.getCommandBuffer();

		ImageViewInstance & view = *ei.target->instance();
		ImageInstance & img = *ei.target->image()->instance();

		synch.addSynch(Resource{
			._image = ei.target,
			._begin_state = {
				.access = VK_ACCESS_2_TRANSFER_READ_BIT,
				.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				.stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
			},
			._end_state = ResourceState2{
				.access = VK_ACCESS_2_TRANSFER_READ_BIT,
				.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
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
		_offset(ci.offset),
		_use_update_buffer_ifp(ci.use_update_buffer_ifp)
	{}

	void UploadBuffer::execute(ExecutionContext& ctx, UploadInfo const& ui)
	{
		if(ui.sources.empty())	return;
		CommandBuffer& cmd = *ctx.getCommandBuffer();


		const bool use_update = [&](){
			bool res = ui.use_update_buffer_ifp.value();
			if (res)
			{
				const uint32_t max_size = 65536;
				for (const auto& src : ui.sources)
				{
					res &= (src.obj.size() <= max_size);
					res &= (src.obj.size() % 4 == 0);
				}
			}
			return res;
		}();
		// first consider .len as .end
		Buffer::Range buffer_range {.begin = size_t(-1), .len = 0};
		for (const auto& src : ui.sources)
		{
			buffer_range.begin = std::min(buffer_range.begin, src.pos);
			buffer_range.len = std::max(buffer_range.len, src.obj.size() + src.pos);
		}
		// now .len is .len
		buffer_range.len = buffer_range.len - buffer_range.begin;

		SynchronizationHelper synch(ctx);
		synch.addSynch(Resource{
			._buffer = ui.dst,
			._buffer_range = buffer_range,
			._begin_state = {
				.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
			},
		});

		if (use_update)
		{
			synch.record();
			for (const auto& src : ui.sources)
			{
				vkCmdUpdateBuffer(cmd, *ui.dst->instance(), src.pos, src.obj.size(), src.obj.data());
			}
		}
		else // Use Staging Buffer
		{
			std::shared_ptr<StagingBuffer> sb = std::make_shared<StagingBuffer>(ctx.stagingPool(), buffer_range.len);
			BufferInstance& sbbi = *sb->buffer()->instance();
			

			// Copy to Staging Buffer
			{
				sbbi.map();
				for (const auto& src : ui.sources)
				{
					std::memcpy(static_cast<uint8_t*>(sbbi.data()) + src.pos, src.obj.data(), src.obj.size());
				}
				sbbi.unMap();

				SynchronizationHelper synch2(ctx);
				synch2.addSynch(Resource{
					._buffer = sb->buffer(),
					._buffer_range = buffer_range,
					._begin_state = {
						.access = VK_ACCESS_2_HOST_WRITE_BIT,
						.stage = VK_PIPELINE_STAGE_2_HOST_BIT,
					},
				});
				synch2.record();
			}

			synch.addSynch(Resource{
				._buffer = sb->buffer(),
				._buffer_range = buffer_range,
				._begin_state = {
					.access = VK_ACCESS_2_TRANSFER_READ_BIT,
					.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				},
			});
			synch.record();

			std::vector<VkBufferCopy2> regions(ui.sources.size());
			for (size_t r = 0; r < regions.size(); ++r)
			{
				regions[r] = VkBufferCopy2{
					.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
					.pNext = nullptr,
					.srcOffset = ui.sources[r].pos,
					.dstOffset = ui.sources[r].pos,
					.size = ui.sources[r].obj.size(),
				};
			}
			
			VkCopyBufferInfo2 copy{
				.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
				.pNext = nullptr,
				.srcBuffer = sbbi,
				.dstBuffer = *ui.dst->instance(),
				.regionCount = static_cast<uint32_t>(regions.size()),
				.pRegions = regions.data(),
			};

			vkCmdCopyBuffer2(cmd, &copy);

			ctx.keppAlive(sb);
		}
	}

	void UploadBuffer::execute(ExecutionContext& ctx)
	{
		UploadInfo ui{
			.sources = {PositionedObjectView{.obj = _src, .pos = _offset.valueOr(0), }},
			.dst = _dst,
			.use_update_buffer_ifp = _use_update_buffer_ifp,
		};
		execute(ctx, ui);
	}

	Executable UploadBuffer::with(UploadInfo const& ui)
	{
		UploadInfo _ui{
			.sources = (!ui.sources.empty()) ? ui.sources : std::vector{PositionedObjectView{.obj = _src, .pos = _offset.valueOr(0), }},
			.dst = ui.dst ? ui.dst : _dst,
			.use_update_buffer_ifp = ui.use_update_buffer_ifp.value_or(_use_update_buffer_ifp),
		};
		return [=](ExecutionContext & ctx) {
			execute(ctx, _ui);
		};
	}



	UploadImage::UploadImage(CreateInfo const& ci):
		TransferCommand(ci.app, ci.name),
		_src(ci.src),
		_dst(ci.dst)
	{}

	void UploadImage::execute(ExecutionContext& ctx, UploadInfo const& ui)
	{
		CommandBuffer& cmd = *ctx.getCommandBuffer();

		SynchronizationHelper synch(ctx);
		synch.addSynch(Resource{
			._image = ui.dst,
			._begin_state = {
				.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
			},
		});

		{
			std::shared_ptr<StagingBuffer> sb = std::make_shared<StagingBuffer>(ctx.stagingPool(), ui.src.size());
			BufferInstance& sbbi = *sb->buffer()->instance();


			// Copy to Staging Buffer
			{
				sbbi.map();
				std::memcpy(sbbi.data(), ui.src.data(), ui.src.size());
				sbbi.unMap();

				SynchronizationHelper synch2(ctx);
				synch2.addSynch(Resource{
					._buffer = sb->buffer(),
					._buffer_range = Range_st{.begin = 0, .len = ui.src.size(), },
					._begin_state = {
						.access = VK_ACCESS_2_HOST_WRITE_BIT,
						.stage = VK_PIPELINE_STAGE_2_HOST_BIT,
					},
				});
				synch2.record();
			}

			synch.addSynch(Resource{
				._buffer = sb->buffer(),
				._buffer_range = Range_st{.begin = 0, .len = ui.src.size(), },
				._begin_state = {
					.access = VK_ACCESS_2_TRANSFER_READ_BIT,
					.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				},
			});
			synch.record();

			VkBufferImageCopy2 region{
				.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
				.pNext = nullptr,
				.bufferOffset = 0,
				.bufferRowLength = 0,   // 0 => tightly packed
				.bufferImageHeight = 0, // 0 => tightly packed
				.imageSubresource = getImageLayersFromRange(ui.dst->instance()->createInfo().subresourceRange),
				.imageOffset = makeZeroOffset3D(),
				.imageExtent = ui.dst->image()->instance()->createInfo().extent,
			};

			VkCopyBufferToImageInfo2 copy{
				.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
				.pNext = nullptr,
				.srcBuffer = sbbi,
				.dstImage = *ui.dst->image()->instance(),
				.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.regionCount = 1,
				.pRegions = &region,
			};

			vkCmdCopyBufferToImage2(cmd, &copy);

			ctx.keppAlive(sb);
		}
	}


	void UploadImage::execute(ExecutionContext& ctx)
	{
		UploadInfo ui{
			.src = _src,
			.dst = _dst,
		};
		execute(ctx, ui);
	}

	Executable UploadImage::with(UploadInfo const& ui)
	{
		UploadInfo _ui{
			.src = ui.src.hasValue() ? ui.src : _src,
			.dst = ui.dst ? ui.dst : _dst,
		};
		return [=](ExecutionContext& ctx) {
			execute(ctx, _ui);
		};
	}




	UploadMesh::UploadMesh(CreateInfo const& ci) :
		TransferCommand(ci.app, ci.name),
		_mesh(ci.mesh)
	{

	}

	void UploadMesh::execute(ExecutionContext& ctx, UploadInfo const& ui)
	{
		assert(!!ui.mesh);

		Mesh::ResourcesToUpload resources = ui.mesh->getResourcesToUpload();

		if (!resources.images.empty())
		{
			UploadImage uploader(UploadImage::CI{
				.app = application(),
				.name = name() + ".ImageUploader",
			});

			for (auto& image_upload : resources.images)
			{
				uploader.execute(ctx, UploadImage::UI{
					.src = image_upload.src,
					.dst = image_upload.dst,
				});
			}
		}

		if (!resources.buffers.empty())
		{
			UploadBuffer uploader(UploadBuffer::CI{
				.app = application(),
				.name = name() + ".BufferUploader",
			});

			for (auto& buffer_upload : resources.buffers)
			{
				uploader.execute(ctx, UploadBuffer::UI{
					.sources = buffer_upload.sources,
					.dst = buffer_upload.dst,
					.use_update_buffer_ifp = false,
				});
			}
		}

		ui.mesh->notifyDeviceDataIsUpToDate();
	}

	void UploadMesh::execute(ExecutionContext& ctx)
	{
		UploadInfo ui{
			.mesh = _mesh,
		};
		execute(ctx, ui);
	}

	Executable UploadMesh::with(UploadInfo const& ui)
	{
		const UploadInfo _ui{
			.mesh = ui.mesh ? ui.mesh : _mesh,
		};
		return [this, _ui](ExecutionContext & ctx)
		{
			execute(ctx, _ui);
		};
	}
}