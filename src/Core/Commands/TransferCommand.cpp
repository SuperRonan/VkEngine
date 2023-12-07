#include "TransferCommand.hpp"

namespace vkl
{
	

	CopyImage::CopyImage(CreateInfo const& ci):
		TransferCommand(ci.app, ci.name),
		_src(ci.src),
		_dst(ci.dst),
		_regions(ci.regions)
	{}

	void CopyImage::execute(ExecutionContext& ctx, CopyInfo const& cinfo)
	{
		std::shared_ptr<CommandBuffer> cmd = ctx.getCommandBuffer();

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
		ctx.keppAlive(cinfo.src->instance());
		ctx.keppAlive(cinfo.dst->instance());
	}

	ExecutionNode CopyImage::getExecutionNode(RecordContext& ctx, CopyInfo const& ci)
	{
		Resources resources = {
			Resource{
				._image = ci.src,
				._begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_READ_BIT,
					.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
				._image_usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			},
			Resource{
				._image = ci.dst,
				._begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT
				},
				._image_usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			},
		};
		CopyInfo ci_copy = ci;
		ExecutionNode res = ExecutionNode::CI{
			.name = name(),
			.resources = resources,
			.exec_fn = [this, ci_copy](ExecutionContext& ctx)
			{
				execute(ctx, ci_copy);
			},
		};
		return res;
	}

	ExecutionNode CopyImage::getExecutionNode(RecordContext& ctx)
	{
		return getExecutionNode(ctx, getDefaultCopyInfo());
	}

	Executable CopyImage::with(CopyInfo const& ci)
	{
		return [this, ci](RecordContext& ctx)
		{
			CopyInfo cinfo{
				.src = ci.src ? ci.src : _src,
				.dst = ci.dst ? ci.dst : _dst,
				.regions = ci.regions.empty() ? _regions : ci.regions,
			};
			return getExecutionNode(ctx, cinfo);
		};
	}


	CopyBufferToImage::CopyBufferToImage(CreateInfo const& ci) :
		TransferCommand(ci.app, ci.name),
		_src(ci.src),
		_range(ci.range),
		_dst(ci.dst),
		_regions(ci.regions)
	{}

	void CopyBufferToImage::execute(ExecutionContext& context, CopyInfo const& cinfo)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		
		uint32_t n_regions = static_cast<uint32_t>(cinfo.regions.size());
		const VkBufferImageCopy* p_regions = cinfo.regions.data();
		VkBufferImageCopy _reg;
		if (n_regions == 0)
		{
			const VkExtent3D extent = *cinfo.dst->image()->extent();
			_reg = VkBufferImageCopy{
				.bufferOffset = 0,
				.bufferRowLength = cinfo.default_buffer_row_length,   // 0 => tightly packed
				.bufferImageHeight = cinfo.default_buffer_image_height, // 0 => tightly packed
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
		context.keppAlive(cinfo.src->instance());
		context.keppAlive(cinfo.dst->instance());
	}

	ExecutionNode CopyBufferToImage::getExecutionNode(RecordContext& ctx, CopyInfo const& ci)
	{
		Resources resources = {
			Resource{
				._buffer = ci.src,
				._buffer_range = ci.range,
				._begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_READ_BIT,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
				._buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			},
			Resource{
				._image = ci.dst,
				._begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
				._image_usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			},
		};
		CopyInfo ci_copy = ci;
		ExecutionNode res = ExecutionNode::CI{
			.name = name(),
			.resources = resources,
			.exec_fn = [this, ci_copy](ExecutionContext& ctx)
			{
				execute(ctx, ci_copy);
			},
		};
		return res;
	}

	ExecutionNode CopyBufferToImage::getExecutionNode(RecordContext& ctx)
	{
		return getExecutionNode(ctx, getDefaultCopyInfo());
	}

	Executable CopyBufferToImage::with(CopyInfo const& info)
	{
		return [this, info](RecordContext& context)
		{
			const CopyInfo cinfo{
				.src = info.src ? info.src : _src,
				.range = info.range.len ? info.range : _range.value(),
				.dst = info.dst ? info.dst : _dst,
				.regions = info.regions.empty() ? _regions : info.regions,
				.default_buffer_row_length = info.default_buffer_row_length,
				.default_buffer_image_height = info.default_buffer_image_height,
			};
			return getExecutionNode(context, cinfo);
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

	void CopyBuffer::execute(ExecutionContext& context, CopyInfo const& cinfo)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();

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
		context.keppAlive(cinfo.src->instance());
		context.keppAlive(cinfo.dst->instance());
	}

	ExecutionNode CopyBuffer::getExecutionNode(RecordContext& ctx, CopyInfo const& ci)
	{
		Resources resources{
			Resource{
				._buffer = ci.src,
				._buffer_range = Range_st{.begin = ci.src_offset, .len = ci.size},
				._begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_READ_BIT,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
				._buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			},
			Resource{
				._buffer = ci.dst,
				._buffer_range = Range_st{.begin = ci.dst_offset, .len = ci.size},
				._begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
				._buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			},
		};
		CopyInfo ci_copy = ci;
		ExecutionNode res = ExecutionNode::CI{
			.name = name(),
			.resources = resources,
			.exec_fn = [this, ci_copy](ExecutionContext& ctx)
			{
				execute(ctx, ci_copy);
			},
		};
		return res;
	}

	ExecutionNode CopyBuffer::getExecutionNode(RecordContext& ctx)
	{
		return getExecutionNode(ctx, getDefaultCopyInfo());
	}

	Executable CopyBuffer::with(CopyInfo const& cinfo)
	{
		return [this, cinfo](RecordContext& ctx)
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
			return getExecutionNode(ctx, ci);
		};
	}


	FillBuffer::FillBuffer(CreateInfo const& ci) :
		TransferCommand(ci.app, ci.name),
		_buffer(ci.buffer),
		_range(ci.range),
		_value(ci.value)
	{

	}

	void FillBuffer::execute(ExecutionContext& context, FillInfo const& fi)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		
		VkDeviceSize len = fi.range.value().len;
		if (len == 0)
		{
			len = fi.buffer->instance()->createInfo().size;
		}
		vkCmdFillBuffer(*cmd, *fi.buffer->instance(), fi.range.value().begin, len, fi.value.value());
		context.keppAlive(fi.buffer->instance());
	}

	ExecutionNode FillBuffer::getExecutionNode(RecordContext& ctx, FillInfo const& fi)
	{
		Resources resources = {
			Resource{
				._buffer = fi.buffer,
				._buffer_range = fi.range.value(),
				._begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.stage = VK_PIPELINE_STAGE_2_CLEAR_BIT,
				},
				._buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			},
		};
		FillInfo fi_copy = fi;
		ExecutionNode res = ExecutionNode::CI{
			.name = name(),
			.resources = resources,
			.exec_fn = [this, fi_copy](ExecutionContext& ctx)
			{
				execute(ctx, fi_copy);
			},
		};
		return res;
	}

	ExecutionNode FillBuffer::getExecutionNode(RecordContext& ctx)
	{
		return getExecutionNode(ctx, getDefaultFillInfo());
	}

	Executable FillBuffer::with(FillInfo const& fi)
	{
		return [this, fi](RecordContext& context)
		{
			const FillInfo _fi{
				.buffer = fi.buffer ? fi.buffer : _buffer,
				.range = fi.range.has_value() ? fi.range.value() : _range.value(),
				.value = fi.value.value_or(_value),
			};
			return getExecutionNode(context, fi);
		};
	}



	ClearImage::ClearImage(CreateInfo const& ci):
		TransferCommand(ci.app, ci.name),
		_view(ci.view),
		_value(ci.value)
	{

	}

	void ClearImage::execute(ExecutionContext& context, ClearInfo const& ci)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();

		const VkImageSubresourceRange range = ci.view->range();

		const VkClearValue value = ci.value.value();

		VkImageAspectFlags aspect = ci.view->range().aspectMask;

		// TODO manage other aspects
		if (aspect & VK_IMAGE_ASPECT_COLOR_BIT)
		{
			vkCmdClearColorImage(
				*cmd, 
				*ci.view->image()->instance(), 
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
				&value.color, 
				1, 
				&range
			);
		}
		if (aspect & VK_IMAGE_ASPECT_DEPTH_BIT)
		{
			vkCmdClearDepthStencilImage(
				*cmd, 
				ci.view->image()->instance()->handle(), 
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
				&value.depthStencil,
				1, 
				&range
			);
		}
		context.keppAlive(ci.view->instance());
	}

	ExecutionNode ClearImage::getExecutionNode(RecordContext& ctx, ClearInfo const& ci)
	{
		Resources resources = {
			Resource{
				._image = ci.view,
				._begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.stage = VK_PIPELINE_STAGE_2_CLEAR_BIT,
				},
				._image_usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			},
		};
		ClearInfo ci_copy = ci;
		ExecutionNode res = ExecutionNode::CI{
			.name = name(),
			.resources = resources,
			.exec_fn = [this, ci_copy](ExecutionContext& ctx)
			{
				execute(ctx, ci_copy);
			},
		};
		return res;
	}

	ExecutionNode ClearImage::getExecutionNode(RecordContext& ctx)
	{
		return getExecutionNode(ctx, getDefaultClearInfo());
	}

	Executable ClearImage::with(ClearInfo const& ci)
	{
		return [this, ci](RecordContext& context)
		{
			const ClearInfo cinfo{
				.view = ci.view ? ci.view : _view,
				.value = ci.value.value_or(_value),
			};
			return getExecutionNode(context, ci);
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
		vkCmdUpdateBuffer(*ctx.getCommandBuffer(), *ui.dst->instance(), ui.offset.value(), ui.src.size(), ui.src.data());
		ctx.keppAlive(ui.dst->instance());
	}

	ExecutionNode UpdateBuffer::getExecutionNode(RecordContext& ctx, UpdateInfo const& ui)
	{
		Resources resources = {
			Resource{
				._buffer = ui.dst,
				._buffer_range = Range_st{.begin = ui.offset.value(), .len = ui.src.size(), },
				._begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
				._buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			},
		};
		UpdateInfo ui_copy = ui;
		ExecutionNode res = ExecutionNode::CI{
			.name = name(),
			.resources = resources,
			.exec_fn = [this, ui_copy](ExecutionContext& ctx)
			{
				execute(ctx, ui_copy);
			},
		};
		return res;
	}

	ExecutionNode UpdateBuffer::getExecutionNode(RecordContext& ctx)
	{
		return getExecutionNode(ctx, getDefaultUpdateInfo());
	}

	Executable UpdateBuffer::with(UpdateInfo const& ui)
	{
		return [this, ui](RecordContext& ctx)
		{
			UpdateInfo _ui
			{
				.src = ui.src.hasValue() ? ui.src : _src,
				.dst = ui.dst ? ui.dst : _dst,
				.offset = ui.offset.has_value() ? ui.offset.value() : _offset.value(),
			};

			return getExecutionNode(ctx, _ui);
		};
	}
	


	



	UploadBuffer::UploadBuffer(CreateInfo const& ci):
		TransferCommand(ci.app, ci.name),
		_src(ci.src),
		_dst(ci.dst),
		_offset(ci.offset),
		_use_update_buffer_ifp(ci.use_update_buffer_ifp)
	{}

	void UploadBuffer::execute(ExecutionContext& ctx, UploadInfo const& ui, bool use_update, Buffer::Range buffer_range)
	{
		if(ui.sources.empty())	return;
		CommandBuffer& cmd = *ctx.getCommandBuffer();
		

		if (use_update)
		{
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
				//VkBufferMemoryBarrier2 sb_barrier{
				//	.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
				//	.pNext = nullptr,
				//	.srcStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
				//};
				//VkDependencyInfo dependency{
				//	.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				//	.pNext = nullptr,
				//	.dependencyFlags = 0,
				//	.memoryBarrierCount = 0,
				//	.pMemoryBarriers = nullptr,
				//	.bufferMemoryBarrierCount = 1,
				//	.pBufferMemoryBarriers = &sb_barrier,
				//	.imageMemoryBarrierCount = 0,
				//	.pImageMemoryBarriers = nullptr,
				//};
				//vkCmdPipelineBarrier2(cmd, &dependency);

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

				sbbi.map();
				for (const auto& src : ui.sources)
				{
					std::memcpy(static_cast<uint8_t*>(sbbi.data()) + src.pos, src.obj.data(), src.obj.size());
				}
				vmaFlushAllocation(sb->buffer()->instance()->allocator(), sb->buffer()->instance()->allocation(), buffer_range.begin, buffer_range.len);
				// Flush each subrange individually?
				sbbi.unMap();

				
			}

			SynchronizationHelper synch(ctx);
			synch.addSynch(Resource{
				._buffer = sb->buffer(),
				._buffer_range = buffer_range,
				._begin_state = {
					.access = VK_ACCESS_2_TRANSFER_READ_BIT,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
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
			ctx.keppAlive(ui.dst->instance());
		}
	}

	ExecutionNode UploadBuffer::getExecutionNode(RecordContext& ctx, UploadInfo const& ui)
	{
		const bool use_update = [&]() {
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
		Buffer::Range buffer_range{ .begin = size_t(-1), .len = 0 };
		for (const auto& src : ui.sources)
		{
			buffer_range.begin = std::min(buffer_range.begin, src.pos);
			buffer_range.len = std::max(buffer_range.len, src.obj.size() + src.pos);
		}
		// now .len is .len
		buffer_range.len = buffer_range.len - buffer_range.begin;
		const bool merge_synch = true;
		Resources resources;
		if(merge_synch)
		{
			resources.push_back(Resource{
				._buffer = ui.dst,
				._buffer_range = buffer_range,
				._begin_state = {
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
			});
		}
		else
		{
			for (const auto& src : ui.sources)
			{
				resources.push_back(Resource{
					._buffer = ui.dst,
					._buffer_range = Range_st{.begin = src.pos, .len = src.obj.size()},
					._begin_state = {
						.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
						.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
					},
				});
			}
		}
		
		UploadInfo ui_copy = ui;
		ExecutionNode res = ExecutionNode::CI{
			.name = name(),
			.resources = resources,
			.exec_fn = [this, ui_copy, use_update, buffer_range](ExecutionContext& ctx)
			{
				execute(ctx, ui_copy, use_update, buffer_range);
			},
		};
		return res;
	}

	ExecutionNode UploadBuffer::getExecutionNode(RecordContext& ctx)
	{
		return getExecutionNode(ctx, getDefaultUploadInfo());
	}


	Executable UploadBuffer::with(UploadInfo const& ui)
	{
		return [this, ui](RecordContext & ctx)
		{
			UploadInfo _ui{
				.sources = (!ui.sources.empty()) ? ui.sources : std::vector{PositionedObjectView{.obj = _src, .pos = _offset.valueOr(0), }},
				.dst = ui.dst ? ui.dst : _dst,
				.use_update_buffer_ifp = ui.use_update_buffer_ifp.value_or(_use_update_buffer_ifp),
			};
			return getExecutionNode(ctx, ui);
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

		std::shared_ptr<StagingBuffer> sb = std::make_shared<StagingBuffer>(ctx.stagingPool(), ui.src.size());
		BufferInstance& sbbi = *sb->buffer()->instance();


		// Copy to Staging Buffer
		{
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
				
			sbbi.map();
			std::memcpy(sbbi.data(), ui.src.data(), ui.src.size());
			vmaFlushAllocation(sb->buffer()->instance()->allocator(), sb->buffer()->instance()->allocation(), 0, ui.src.size());
			sbbi.unMap();
		}

		SynchronizationHelper synch(ctx);
		synch.addSynch(Resource{
			._buffer = sb->buffer(),
			._buffer_range = Range_st{.begin = 0, .len = ui.src.size(), },
			._begin_state = {
				.access = VK_ACCESS_2_TRANSFER_READ_BIT,
				.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
			},
		});
		synch.record();

		VkBufferImageCopy2 region{
			.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
			.pNext = nullptr,
			.bufferOffset = 0,
			.bufferRowLength = ui.buffer_row_length,   // 0 => tightly packed
			.bufferImageHeight = ui.buffer_image_height, // 0 => tightly packed
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
		ctx.keppAlive(ui.dst->instance());
		
	}

	ExecutionNode UploadImage::getExecutionNode(RecordContext& ctx, UploadInfo const& ui)
	{
		Resources resources = {
			Resource{
				._image = ui.dst,
				._begin_state = {
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
			},
		};
		UploadInfo ui_copy = ui;
		ExecutionNode res = ExecutionNode::CI{
			.name = name(),
			.resources = resources,
			.exec_fn = [this, ui_copy](ExecutionContext& ctx)
			{
				execute(ctx, ui_copy);
			},
		};
		return res;
	}

	ExecutionNode UploadImage::getExecutionNode(RecordContext& ctx)
	{
		return getExecutionNode(ctx, getDefaultUploadInfo());
	}

	Executable UploadImage::with(UploadInfo const& ui)
	{
		return [this, ui](RecordContext& ctx) {
			
			UploadInfo _ui{
				.src = ui.src.hasValue() ? ui.src : _src,
				.dst = ui.dst ? ui.dst : _dst,
			};
			return getExecutionNode(ctx, _ui);
		};
	}




	UploadResources::UploadResources(CreateInfo const& ci) :
		TransferCommand(ci.app, ci.name),
		_holder(ci.holder)
	{

	}

	class CallbackHolder : public VkObject
	{
	public:

		int value = 0;

		std::vector<CompletionCallback> callbacks;

		virtual ~CallbackHolder() override
		{
			for (CompletionCallback& cb : callbacks)
			{
				cb(value);
			}
		}
	};

	void UploadResources::execute(ExecutionContext& ctx, UploadInfo const& ui, std::vector<BufferUploadExtraInfo> const& extra_buffer_info)
	{
		const ResourcesToUpload & resources = ui.upload_list;

		UploadImage image_uploader(UploadImage::CI{
			.app = application(),
			.name = name() + ".ImageUploader",
		});

		UploadBuffer buffer_uploader(UploadBuffer::CI{
			.app = application(),
			.name = name() + ".BufferUploader",
		});

		std::shared_ptr<CallbackHolder> completion_callbacks;

		for (size_t i = 0; i < resources.buffers.size(); ++i)
		{
			auto& buffer_upload = resources.buffers[i];
			buffer_uploader.execute(ctx, UploadBuffer::UI{
				.sources = buffer_upload.sources,
				.dst = buffer_upload.dst,
				.use_update_buffer_ifp = false,
			}, extra_buffer_info[i].use_update, extra_buffer_info[i].range);
			if (buffer_upload.completion_callback)
			{
				if(!completion_callbacks) completion_callbacks = std::make_shared<CallbackHolder>();
				completion_callbacks->callbacks.push_back(buffer_upload.completion_callback);
			}
		}
		
		for (auto& image_upload : resources.images)
		{
			image_uploader.execute(ctx, UploadImage::UI{
				.src = image_upload.src,
				.buffer_row_length = image_upload.buffer_row_length,
				.buffer_image_height = image_upload.buffer_image_height,
				.dst = image_upload.dst,
			});
			if (image_upload.completion_callback)
			{
				if (!completion_callbacks) completion_callbacks = std::make_shared<CallbackHolder>();
				completion_callbacks->callbacks.push_back(image_upload.completion_callback);
			}
		}

		if (completion_callbacks.operator bool() && !completion_callbacks->callbacks.empty())
		{
			ctx.keppAlive(completion_callbacks);
		}

	}

	ExecutionNode UploadResources::getExecutionNode(RecordContext& ctx, UploadInfo const& ui)
	{
		// Assuming no aliasing between resources
		Resources resources;
		resources.reserve(ui.upload_list.buffers.size() + ui.upload_list.images.size());
		std::vector<BufferUploadExtraInfo> extra_buffer_info(ui.upload_list.buffers.size());
		
		for (size_t i = 0; i<ui.upload_list.buffers.size(); ++i)
		{
			const auto & buffer_upload = ui.upload_list.buffers[i];

			const bool use_update = [&]() {
				bool res = true;
				if (res)
				{
					const uint32_t max_size = 65536;
					for (const auto& src : buffer_upload.sources)
					{
						res &= (src.obj.size() <= max_size);
						res &= (src.obj.size() % 4 == 0);
					}
				}
				return res;
			}();

			// first consider .len as .end
			Buffer::Range buffer_range{ .begin = size_t(-1), .len = 0 };
			for (const auto& src : buffer_upload.sources)
			{
				buffer_range.begin = std::min(buffer_range.begin, src.pos);
				buffer_range.len = std::max(buffer_range.len, src.obj.size() + src.pos);
			}
			// now .len is .len
			buffer_range.len = buffer_range.len - buffer_range.begin;
			const bool merge_synch = true;

			extra_buffer_info[i] = {
				.use_update = use_update,
				.range = buffer_range,
			};

			if (merge_synch)
			{
				resources.push_back(Resource{
					._buffer = buffer_upload.dst,
					._buffer_range = buffer_range,
					._begin_state = {
						.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
						.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
					},
				});
			}
			else
			{
				for (const auto& src : buffer_upload.sources)
				{
					resources.push_back(Resource{
						._buffer = buffer_upload.dst,
						._buffer_range = Range_st{.begin = src.pos, .len = src.obj.size()},
						._begin_state = {
							.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
							.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
						},
					});
				}
			}
		}
		for (const auto& image_upload : ui.upload_list.images)
		{
			resources.push_back(Resource{
				._image = image_upload.dst,
				._begin_state = {
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
			});
		}
		UploadInfo ui_copy = ui;
		ExecutionNode res = ExecutionNode::CI{
			.name = name(),
			.resources = resources,
			.exec_fn = [this, ui_copy, extra_buffer_info](ExecutionContext& ctx)
			{
				execute(ctx, ui_copy, extra_buffer_info);
			},
		};
		return res;
	}

	ExecutionNode UploadResources::getExecutionNode(RecordContext& ctx)
	{
		UploadInfo ui{
			.upload_list = _holder->getResourcesToUpload(),
		};
		return getExecutionNode(ctx, ui);
	}

	Executable UploadResources::with(UploadInfo const& ui)
	{
		return [this, ui](RecordContext & ctx)
		{
			return getExecutionNode(ctx, ui);
		};
	}
}