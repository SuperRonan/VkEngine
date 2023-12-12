#include "TransferCommand.hpp"

namespace vkl
{
	

	CopyImage::CopyImage(CreateInfo const& ci):
		TransferCommand(ci.app, ci.name),
		_src(ci.src),
		_dst(ci.dst),
		_regions(ci.regions)
	{}

	void CopyImage::execute(ExecutionContext& ctx, CopyInfoInstance const& cinfo)
	{
		std::shared_ptr<CommandBuffer> cmd = ctx.getCommandBuffer();

		const VkImageCopy* regions = cinfo.regions.data();
		uint32_t n_regions = static_cast<uint32_t>(cinfo.regions.size());
		VkImageCopy _region;
		if (cinfo.regions.empty())
		{
			_region = VkImageCopy{
				.srcSubresource = getImageLayersFromRange(cinfo.src->createInfo().subresourceRange),
				.srcOffset = makeZeroOffset3D(),
				.dstSubresource = getImageLayersFromRange(cinfo.dst->createInfo().subresourceRange),
				.dstOffset = makeZeroOffset3D(),
				.extent = cinfo.dst->image()->createInfo().extent,
			};
			regions = &_region;
			n_regions = 1;
		}

		vkCmdCopyImage(*cmd,
			*cinfo.src->image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			*cinfo.dst->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			n_regions, regions
		);
	}

	ExecutionNode CopyImage::getExecutionNode(RecordContext& ctx, CopyInfo const& ci)
	{
		ResourcesInstances resources = {
			ResourceInstance{
				.image_view = ci.src->instance(),
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_READ_BIT,
					.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
				.image_usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			},
			ResourceInstance{
				.image_view = ci.dst->instance(),
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT
				},
				.image_usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			},
		};
		CopyInfoInstance cii{
			.src = ci.src->instance(),
			.dst = ci.dst->instance(),
			.regions = ci.regions,
		};
		ExecutionNode res = ExecutionNode::CI{
			.name = name(),
			.resources = resources,
			.exec_fn = [this, cii](ExecutionContext& ctx)
			{
				execute(ctx, cii);
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

	void CopyBufferToImage::execute(ExecutionContext& context, CopyInfoInstance const& cinfo)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		
		uint32_t n_regions = static_cast<uint32_t>(cinfo.regions.size());
		const VkBufferImageCopy* p_regions = cinfo.regions.data();
		VkBufferImageCopy _reg;
		if (n_regions == 0)
		{
			const VkExtent3D extent = cinfo.dst->image()->createInfo().extent;
			_reg = VkBufferImageCopy{
				.bufferOffset = 0,
				.bufferRowLength = cinfo.default_buffer_row_length,   // 0 => tightly packed
				.bufferImageHeight = cinfo.default_buffer_image_height, // 0 => tightly packed
				.imageSubresource = getImageLayersFromRange(cinfo.dst->createInfo().subresourceRange),
				.imageOffset = makeZeroOffset3D(),
				.imageExtent = extent,
			};
			p_regions = &_reg;
			n_regions = 1;
		}

		vkCmdCopyBufferToImage(
			*cmd,
			*cinfo.src, *cinfo.dst->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			n_regions, p_regions
		);
	}

	ExecutionNode CopyBufferToImage::getExecutionNode(RecordContext& ctx, CopyInfo const& ci)
	{
		ResourcesInstances resources = {
			ResourceInstance{
				.buffer = ci.src->instance(),
				.buffer_range = ci.range,
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_READ_BIT,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
				.buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			},
			ResourceInstance{
				.image_view = ci.dst->instance(),
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
				.image_usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			},
		};
		CopyInfoInstance cii{
			.src = ci.src->instance(),
			.range = ci.range,
			.dst = ci.dst->instance(),
			.regions = ci.regions,
			.default_buffer_row_length = ci.default_buffer_row_length,
			.default_buffer_image_height = ci.default_buffer_image_height,
		};
		ExecutionNode res = ExecutionNode::CI{
			.name = name(),
			.resources = resources,
			.exec_fn = [this, cii](ExecutionContext& ctx)
			{
				execute(ctx, cii);
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

	void CopyBuffer::execute(ExecutionContext& context, CopyInfoInstance const& cinfo)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();

		VkCopyBufferInfo2 copy{
			.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
			.pNext = nullptr,
			.srcBuffer = cinfo.src->handle(),
			.dstBuffer = cinfo.dst->handle(),
			.regionCount = static_cast<uint32_t>(cinfo.regions.size()),
			.pRegions = cinfo.regions.data(),
		};

		vkCmdCopyBuffer2(context.getCommandBuffer()->handle(), &copy);
	}

	ExecutionNode CopyBuffer::getExecutionNode(RecordContext& ctx, CopyInfo const& ci)
	{
		CopyInfoInstance cii{
			.src = ci.src->instance(),
			.dst = ci.dst->instance(),
			.regions = ci.regions,
		};
		size_t src_base = std::numeric_limits<size_t>::max();
		size_t dst_base = std::numeric_limits<size_t>::max();
		size_t end = 0;

		if (cii.regions.empty())
		{
			cii.regions = {
				VkBufferCopy2{
					.srcOffset = 0,
					.dstOffset = 0,
					.size = std::max(cii.src->createInfo().size, cii.dst->createInfo().size),
				},
			};
		}

		for (size_t i = 0; i < cii.regions.size(); ++i)
		{
			cii.regions[i].sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
			cii.regions[i].pNext = nullptr;

			if (cii.regions[i].size == 0)
			{
				cii.regions[i].size = std::max(cii.src->createInfo().size, cii.dst->createInfo().size);
			}

			src_base = std::min(src_base, ci.regions[i].srcOffset);
			dst_base = std::min(dst_base, ci.regions[i].dstOffset);

			end = std::max(end, ci.regions[i].srcOffset + ci.regions[i].size);
		}
		size_t len = end - src_base;

		ResourcesInstances resources{
			ResourceInstance{
				.buffer = ci.src->instance(),
				.buffer_range = Range_st{.begin = src_base, .len = len},
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_READ_BIT,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
				.buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			},
			ResourceInstance{
				.buffer = ci.dst->instance(),
				.buffer_range = Range_st{.begin = dst_base, .len = len},
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
				.buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			},
		};
		ExecutionNode res = ExecutionNode::CI{
			.name = name(),
			.resources = resources,
			.exec_fn = [this, cii](ExecutionContext& ctx)
			{
				execute(ctx, cii);
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
				.dst = cinfo.dst ? cinfo.dst : _dst,
				.regions = cinfo.regions,
			};
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

	void FillBuffer::execute(ExecutionContext& context, FillInfoInstance const& fi)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		
		vkCmdFillBuffer(*cmd, *fi.buffer, fi.range.begin, fi.range.len, fi.value);
	}

	ExecutionNode FillBuffer::getExecutionNode(RecordContext& ctx, FillInfo const& fi)
	{
		ResourcesInstances resources = {
			ResourceInstance{
				.buffer = fi.buffer->instance(),
				.buffer_range = fi.range.value(),
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.stage = VK_PIPELINE_STAGE_2_CLEAR_BIT,
				},
				.buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			},
		};
		FillInfoInstance fii{
			.buffer = fi.buffer->instance(),
			// range
			.value = fi.value.value(),
		};
		if (!fi.range.has_value() || (fi.range.value().len == 0))
		{
			fii.range = fi.buffer->fullRange().value();
		}
		if (fi.value.has_value() == false)
		{
			fii.value = _value;
		}
		ExecutionNode res = ExecutionNode::CI{
			.name = name(),
			.resources = resources,
			.exec_fn = [this, fii](ExecutionContext& ctx)
			{
				execute(ctx, fii);
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
		ResourcesInstances resources = {
			ResourceInstance{
				.buffer = ui.dst->instance(),
				.buffer_range = Range_st{.begin = ui.offset.value(), .len = ui.src.size(), },
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
				.buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
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

	void UploadBuffer::execute(ExecutionContext& ctx, UploadInfoInstance const& ui)
	{
		if(ui.sources.empty())	return;
		CommandBuffer& cmd = *ctx.getCommandBuffer();
		

		if (ui.use_update)
		{
			for (const auto& src : ui.sources)
			{
				vkCmdUpdateBuffer(cmd, *ui.dst, src.pos, src.obj.size(), src.obj.data());
			}
		}
		else // Use Staging Buffer
		{
			std::shared_ptr<StagingBuffer> sb;
			if (ui.staging_buffer)
			{
				// Assume externally synched
				sb = ui.staging_buffer;
			}
			else
			{
				sb = std::make_shared<StagingBuffer>(ctx.stagingPool(), ui.buffer_range.len);
				{
					SynchronizationHelper synch(ctx);
					synch.addSynch(ResourceInstance{
						.buffer = sb->buffer(),
						.buffer_range = Buffer::Range{.begin = 0, .len = ui.buffer_range.len},
						.begin_state = {
							.access = VK_ACCESS_2_HOST_WRITE_BIT,
							.stage = VK_PIPELINE_STAGE_2_HOST_BIT,
						},
					});
					synch.record();
				}
			}
			// Copy to Staging Buffer
			{
				sb->buffer()->map();
				for (const auto& src : ui.sources)
				{
					std::memcpy(static_cast<uint8_t*>(sb->buffer()->data()) + src.pos, src.obj.data(), src.obj.size());
				}
				vmaFlushAllocation(sb->buffer()->allocator(), sb->buffer()->allocation(), 0, ui.buffer_range.len);
				// Flush each subrange individually? probably slow
				sb->buffer()->unMap();
			}

			if (ui.staging_buffer)
			{
				// Assume externally .end_state
				VkBufferMemoryBarrier2 sb_barrier{
					.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
					.pNext = nullptr,
					.srcStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
					.srcAccessMask = VK_ACCESS_2_HOST_WRITE_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
					.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.buffer = sb->buffer()->handle(),
					.offset = 0,
					.size = ui.buffer_range.len,
				};
				VkDependencyInfo dependency{
					.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
					.pNext = nullptr,
					.dependencyFlags = 0,
					.memoryBarrierCount = 0,
					.pMemoryBarriers = nullptr,
					.bufferMemoryBarrierCount = 1,
					.pBufferMemoryBarriers = &sb_barrier,
					.imageMemoryBarrierCount = 0,
					.pImageMemoryBarriers = nullptr,
				};
				vkCmdPipelineBarrier2(cmd, &dependency);
			}
			else
			{
				SynchronizationHelper synch(ctx);
				synch.addSynch(ResourceInstance{
					.buffer = sb->buffer(),
					.buffer_range = Buffer::Range{.begin = 0, .len = ui.buffer_range.len},
					.begin_state = {
						.access = VK_ACCESS_2_TRANSFER_READ_BIT,
						.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
					},
				});
				synch.record();
			}

			std::vector<VkBufferCopy2> regions(ui.sources.size());
			for (size_t r = 0; r < regions.size(); ++r)
			{
				regions[r] = VkBufferCopy2{
					.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
					.pNext = nullptr,
					.srcOffset = ui.sources[r].pos,
					.dstOffset = ui.sources[r].pos + ui.buffer_range.begin,
					.size = ui.sources[r].obj.size(),
				};
			}
			
			VkCopyBufferInfo2 copy{
				.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
				.pNext = nullptr,
				.srcBuffer = sb->buffer()->handle(),
				.dstBuffer = ui.dst->handle(),
				.regionCount = static_cast<uint32_t>(regions.size()),
				.pRegions = regions.data(),
			};

			vkCmdCopyBuffer2(cmd, &copy);

			//if (!ui.staging_buffer)
			{
				ctx.keppAlive(sb);
			}
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
		ResourcesInstances resources;
		if(merge_synch)
		{
			resources.push_back(ResourceInstance{
				.buffer = ui.dst->instance(),
				.buffer_range = buffer_range,
				.begin_state = {
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
			});
		}
		else
		{
			for (const auto& src : ui.sources)
			{
				resources.push_back(ResourceInstance{
					.buffer = ui.dst->instance(),
					.buffer_range = Buffer::Range{.begin = src.pos, .len = src.obj.size()},
					.begin_state = {
						.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
						.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
					},
				});
			}
		}
		
		UploadInfoInstance uii{
			.sources = ui.sources,
			.dst = ui.dst->instance(),
			.use_update = use_update,
			.buffer_range = buffer_range,
		};

		if (!use_update && ctx.stagingPool())
		{
			std::shared_ptr<StagingBuffer> sb = std::make_shared<StagingBuffer>(ctx.stagingPool(), buffer_range.len);
			uii.staging_buffer = sb;
			resources.push_back(ResourceInstance{
				.buffer = sb->buffer(),
				.buffer_range = Buffer::Range{.begin = 0, .len = buffer_range.len},
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_HOST_WRITE_BIT,
					.stage = VK_PIPELINE_STAGE_2_HOST_BIT,
				},
				.end_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_READ_BIT,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
			});
		}

		ExecutionNode res = ExecutionNode::CI{
			.name = name(),
			.resources = resources,
			.exec_fn = [this, uii](ExecutionContext& ctx)
			{
				execute(ctx, uii);
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

	void UploadImage::execute(ExecutionContext& ctx, UploadInfoInstance const& ui)
	{
		CommandBuffer& cmd = *ctx.getCommandBuffer();

		std::shared_ptr<StagingBuffer> sb;
		if (ui.staging_buffer)
		{
			// Assume externally synched
			sb = ui.staging_buffer;
		}
		else
		{
			sb = std::make_shared<StagingBuffer>(ctx.stagingPool(), ui.src.size());
			
			SynchronizationHelper synch2(ctx);
			synch2.addSynch(ResourceInstance{
				.buffer = sb->buffer(),
				.buffer_range = Buffer::Range{.begin = 0, .len = ui.src.size(), },
				.begin_state = {
					.access = VK_ACCESS_2_HOST_WRITE_BIT,
					.stage = VK_PIPELINE_STAGE_2_HOST_BIT,
				},
			});
			synch2.record();
		}

		// Copy to Staging Buffer
		{
			sb->buffer()->map();
			std::memcpy(sb->buffer()->data(), ui.src.data(), ui.src.size());
			vmaFlushAllocation(sb->buffer()->allocator(), sb->buffer()->allocation(), 0, ui.src.size());
			sb->buffer()->unMap();
		}

		if (ui.staging_buffer)
		{
			// Assume externally .end_state
			VkBufferMemoryBarrier2 sb_barrier{
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
				.pNext = nullptr,
				.srcStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
				.srcAccessMask = VK_ACCESS_2_HOST_WRITE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
				.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.buffer = sb->buffer()->handle(),
				.offset = 0,
				.size = ui.src.size(),
			};
			VkDependencyInfo dependency{
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.pNext = nullptr,
				.dependencyFlags = 0,
				.memoryBarrierCount = 0,
				.pMemoryBarriers = nullptr,
				.bufferMemoryBarrierCount = 1,
				.pBufferMemoryBarriers = &sb_barrier,
				.imageMemoryBarrierCount = 0,
				.pImageMemoryBarriers = nullptr,
			};
			vkCmdPipelineBarrier2(cmd, &dependency);
		}
		else
		{
			SynchronizationHelper synch(ctx);
			synch.addSynch(ResourceInstance{
				.buffer = sb->buffer(),
				.buffer_range = Range_st{.begin = 0, .len = ui.src.size(), },
				.begin_state = {
					.access = VK_ACCESS_2_TRANSFER_READ_BIT,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
			});
			synch.record();
		}

		VkBufferImageCopy2 region{
			.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
			.pNext = nullptr,
			.bufferOffset = 0,
			.bufferRowLength = ui.buffer_row_length,   // 0 => tightly packed
			.bufferImageHeight = ui.buffer_image_height, // 0 => tightly packed
			.imageSubresource = getImageLayersFromRange(ui.dst->createInfo().subresourceRange),
			.imageOffset = makeZeroOffset3D(),
			.imageExtent = ui.dst->image()->createInfo().extent,
		};

		VkCopyBufferToImageInfo2 copy{
			.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
			.pNext = nullptr,
			.srcBuffer = sb->buffer()->handle(),
			.dstImage = ui.dst->image()->handle(),
			.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.regionCount = 1,
			.pRegions = &region,
		};

		vkCmdCopyBufferToImage2(cmd, &copy);

		//if (!ui.staging_buffer)
		{
			ctx.keppAlive(sb);
		}
	}

	ExecutionNode UploadImage::getExecutionNode(RecordContext& ctx, UploadInfo const& ui)
	{
		ResourcesInstances resources = {
			ResourceInstance{
				.image_view = ui.dst->instance(),
				.begin_state = {
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
			},
		};
		UploadInfoInstance uii{
			.src = ui.src,
			.buffer_row_length = ui.buffer_row_length,
			.buffer_image_height = ui.buffer_image_height,
			.dst = ui.dst->instance(),
		};
		if (ctx.stagingPool())
		{
			std::shared_ptr<StagingBuffer> sb = std::make_shared<StagingBuffer>(ctx.stagingPool(), uii.src.size());
			uii.staging_buffer = sb;
			resources.push_back(ResourceInstance{
				.buffer = sb->buffer(),
				.buffer_range = Buffer::Range{.begin = 0, .len = uii.src.size()},
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_HOST_WRITE_BIT,
					.stage = VK_PIPELINE_STAGE_2_HOST_BIT,
				},
				.end_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_READ_BIT,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
			});
		}
		ExecutionNode res = ExecutionNode::CI{
			.name = name(),
			.resources = resources,
			.exec_fn = [this, uii](ExecutionContext& ctx)
			{
				execute(ctx, uii);
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

	void UploadResources::execute(ExecutionContext& ctx, UploadInfo const& ui, std::vector<std::shared_ptr<StagingBuffer>> const& staging_buffers, std::vector<BufferUploadExtraInfo> const& extra_buffer_info)
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

		// TODO use a single staging buffer maybe

		for (size_t i = 0; i < resources.buffers.size(); ++i)
		{
			auto& buffer_upload = resources.buffers[i];
			buffer_uploader.execute(ctx, UploadBuffer::UploadInfoInstance{
				.sources = buffer_upload.sources,
				.dst = buffer_upload.dst,
				.use_update = extra_buffer_info[i].use_update,
				.buffer_range = extra_buffer_info[i].range,
			});
			if (buffer_upload.completion_callback)
			{
				if(!completion_callbacks) completion_callbacks = std::make_shared<CallbackHolder>();
				completion_callbacks->callbacks.push_back(buffer_upload.completion_callback);
			}
		}
		
		for (auto& image_upload : resources.images)
		{
			image_uploader.execute(ctx, UploadImage::UploadInfoInstance{
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
		ResourcesInstances resources;
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
				resources.push_back(ResourceInstance{
					.buffer = buffer_upload.dst,
					.buffer_range = buffer_range,
					.begin_state = {
						.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
						.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
					},
				});
			}
			else
			{
				for (const auto& src : buffer_upload.sources)
				{
					resources.push_back(ResourceInstance{
						.buffer = buffer_upload.dst,
						.buffer_range = Range_st{.begin = src.pos, .len = src.obj.size()},
						.begin_state = {
							.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
							.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
						},
					});
				}
			}
		}
		for (const auto& image_upload : ui.upload_list.images)
		{
			resources.push_back(ResourceInstance{
				.image_view = image_upload.dst,
				.begin_state = {
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
				execute(ctx, ui_copy, {}, extra_buffer_info);
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