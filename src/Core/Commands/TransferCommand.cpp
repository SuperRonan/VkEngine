#include "TransferCommand.hpp"

namespace vkl
{
	

	CopyImage::CopyImage(CreateInfo const& ci):
		TransferCommand(ci.app, ci.name),
		_src(ci.src),
		_dst(ci.dst),
		_regions(ci.regions)
	{}

	struct CopyImageNode : public ExecutionNode
	{
		std::shared_ptr<ImageViewInstance> _src = nullptr;
		std::shared_ptr<ImageViewInstance> _dst = nullptr;
		Array<VkImageCopy> _regions = {};

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;

		CopyImageNode(CreateInfo const& ci) :
			ExecutionNode(ExecutionNode::CI{
				.app = ci.app,
				.name = ci.name,
			})
		{

		}

		virtual void clear() override
		{
			ExecutionNode::clear();
			_src.reset();
			_dst.reset();
			_regions.clear();
		}

		virtual void execute(ExecutionContext & ctx) override
		{
			std::shared_ptr<CommandBuffer> cmd = ctx.getCommandBuffer();

			const VkImageCopy* regions = _regions.data();
			uint32_t n_regions = _regions.size32();
			VkImageCopy _region;
			if (_regions.empty())
			{
				_region = VkImageCopy{
					.srcSubresource = getImageLayersFromRange(_src->createInfo().subresourceRange),
					.srcOffset = makeZeroOffset3D(),
					.dstSubresource = getImageLayersFromRange(_dst->createInfo().subresourceRange),
					.dstOffset = makeZeroOffset3D(),
					.extent = _dst->image()->createInfo().extent,
				};
				regions = &_region;
				n_regions = 1;
			}

			vkCmdCopyImage(*cmd,
				*_src->image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				*_dst->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				n_regions, regions
			);
		}
	};

	std::shared_ptr<ExecutionNode> CopyImage::getExecutionNode(RecordContext& ctx, CopyInfo const& ci)
	{
		std::shared_ptr<CopyImageNode> node = _exec_node_cache.getCleanNode<CopyImageNode>([&]()
		{
			return std::make_shared<CopyImageNode>(CopyImageNode::CI{
				.app = application(),
				.name = name(),
			});
		});

		node->setName(name());

		node->_src = ci.src->instance();
		node->resources() += ImageViewUsage{
			.ivi = node->_src,
			.begin_state = ResourceState2{
				.access = VK_ACCESS_2_TRANSFER_READ_BIT,
				.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
			},
			.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		};

		node->_dst = ci.dst->instance();
		node->resources() += ImageViewUsage{
			.ivi = node->_dst,
			.begin_state = ResourceState2{
				.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.stage = VK_PIPELINE_STAGE_2_COPY_BIT
			},
			.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		};
		return node;
	}

	std::shared_ptr<ExecutionNode> CopyImage::getExecutionNode(RecordContext& ctx)
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

	struct CopyBufferToImageNode : public ExecutionNode
	{
		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;
		CopyBufferToImageNode(CreateInfo const& ci) :
			ExecutionNode(ExecutionNode::CI{
				.app = ci.app,
				.name = ci.name
			})
		{}

		std::shared_ptr<BufferInstance> _src = nullptr;
		Buffer::Range _range;
		std::shared_ptr<ImageViewInstance> _dst = nullptr;
		Array<VkBufferImageCopy> _regions = {};
		uint32_t _default_buffer_row_length = 0;
		uint32_t _default_buffer_image_height = 0;

		virtual void clear() override
		{
			ExecutionNode::clear();
			_src.reset();
			_range = {};
			_dst.reset();
			_regions.clear();
			_default_buffer_row_length = 0;
			_default_buffer_image_height = 0;
		}

		virtual void execute(ExecutionContext& ctx) override
		{
			std::shared_ptr<CommandBuffer> cmd = ctx.getCommandBuffer();

			uint32_t n_regions = _regions.size32();
			const VkBufferImageCopy* p_regions = _regions.data();
			VkBufferImageCopy _reg;
			if (n_regions == 0)
			{
				const VkExtent3D extent = _dst->image()->createInfo().extent;
				_reg = VkBufferImageCopy{
					.bufferOffset = 0,
					.bufferRowLength = _default_buffer_row_length,   // 0 => tightly packed
					.bufferImageHeight = _default_buffer_image_height, // 0 => tightly packed
					.imageSubresource = getImageLayersFromRange(_dst->createInfo().subresourceRange),
					.imageOffset = makeZeroOffset3D(),
					.imageExtent = extent,
				};
				p_regions = &_reg;
				n_regions = 1;
			}

			vkCmdCopyBufferToImage(
				*cmd,
				*_src, *_dst->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				n_regions, p_regions
			);
		}
	};

	std::shared_ptr<ExecutionNode> CopyBufferToImage::getExecutionNode(RecordContext& ctx, CopyInfo const& ci)
	{
		std::shared_ptr<CopyBufferToImageNode> node = _exec_node_cache.getCleanNode<CopyBufferToImageNode>([&]() {
			return std::make_shared<CopyBufferToImageNode>(CopyBufferToImageNode::CI{
				.app = application(),
				.name = name(),
			});
		});
		node->setName(name());

		node->_src = ci.src->instance();
		node->_range = ci.range;
		node->_dst = ci.dst->instance();
		node->_regions = ci.regions;
		node->_default_buffer_row_length = ci.default_buffer_row_length;
		node->_default_buffer_image_height = ci.default_buffer_image_height;

		node->resources() += BufferUsage{
			.bari = {node->_src, node->_range},
			.begin_state = ResourceState2{
				.access = VK_ACCESS_2_TRANSFER_READ_BIT,
				.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
			},
			.usage = VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT_KHR,
		};

		node->resources() += ImageViewUsage{
			.ivi = node->_dst,
			.begin_state = ResourceState2{
				.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
			},
			.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		};

		return node;
	}

	std::shared_ptr<ExecutionNode> CopyBufferToImage::getExecutionNode(RecordContext& ctx)
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

	struct CopyBufferNode : public ExecutionNode
	{
		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;
		CopyBufferNode(CreateInfo const& ci) :
			ExecutionNode(ExecutionNode::CI{
				.app = ci.app,
				.name = ci.name,
			})
		{}

		std::shared_ptr<BufferInstance> _src = nullptr;
		std::shared_ptr<BufferInstance> _dst = nullptr;
		Array<VkBufferCopy2> _regions = {};

		void populate(CopyBuffer::CopyInfo const& ci)
		{
			_src = ci.src->instance();
			_dst = ci.dst->instance();
			_regions = ci.regions;


			size_t src_base = std::numeric_limits<size_t>::max();
			size_t dst_base = std::numeric_limits<size_t>::max();
			size_t end = 0;
			size_t len = 0;
			if (_regions)
			{
				for (size_t i = 0; i < _regions.size(); ++i)
				{
					_regions[i].sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
					_regions[i].pNext = nullptr;

					if (_regions[i].size == 0)
					{
						_regions[i].size = std::max(_src->createInfo().size, _dst->createInfo().size);
					}

					src_base = std::min(src_base, _regions[i].srcOffset);
					dst_base = std::min(dst_base, _regions[i].dstOffset);

					end = std::max(end, _regions[i].srcOffset + _regions[i].size);
				}
				len = end - src_base;
			}
			else
			{
				src_base = 0;
				dst_base = 0;
				len = std::min(_src->createInfo().size, _dst->createInfo().size);
			}
			
			 
			resources() += BufferUsage{
				.bari = {_src, Buffer::Range{.begin = src_base, .len = len}},
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_READ_BIT,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
				.usage = VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT_KHR,
			};
			resources() += BufferUsage{
				.bari = {_dst, Range_st{.begin = dst_base, .len = len}},
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
				.usage = VK_BUFFER_USAGE_2_TRANSFER_DST_BIT_KHR,
			};
		}

		virtual void clear() override
		{
			ExecutionNode::clear();
			_src.reset();
			_dst.reset();
			_regions.clear();
		}

		virtual void execute(ExecutionContext& ctx) override
		{
			std::shared_ptr<CommandBuffer> cmd = ctx.getCommandBuffer();

			const VkBufferCopy2 * regions = _regions.data();
			uint32_t n_regions = _regions.size32();
			VkBufferCopy2 _reg;
			if (n_regions == 0)
			{
				n_regions = 1;
				regions = &_reg;
				_reg = VkBufferCopy2{
					.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
					.pNext = nullptr,
					.srcOffset = 0,
					.dstOffset = 0,
					.size = std::min(_src->createInfo().size, _dst->createInfo().size),
				};
			}

			VkCopyBufferInfo2 copy{
				.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
				.pNext = nullptr,
				.srcBuffer = _src->handle(),
				.dstBuffer = _dst->handle(),
				.regionCount = n_regions,
				.pRegions = regions,
			};

			vkCmdCopyBuffer2(ctx.getCommandBuffer()->handle(), &copy);
		}
	};

	std::shared_ptr<ExecutionNode> CopyBuffer::getExecutionNode(RecordContext& ctx, CopyInfo const& ci)
	{
		std::shared_ptr<CopyBufferNode> node = _exec_node_cache.getCleanNode<CopyBufferNode>([&]() {
			return std::make_shared<CopyBufferNode>(CopyBufferNode::CI{
				.app = application(),
				.name = name(),
			});
		});

		node->setName(name());
		node->populate(ci);

		return node;
	}

	std::shared_ptr<ExecutionNode> CopyBuffer::getExecutionNode(RecordContext& ctx)
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

	struct FillBufferNode : public ExecutionNode
	{
		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;
		FillBufferNode(CreateInfo const& ci) :
			ExecutionNode(ExecutionNode::CI{
				.app = ci.app,
				.name = ci.name,
			})
		{}

		std::shared_ptr<BufferInstance> _buffer = nullptr;
		Buffer::Range _range = {};
		uint32_t _value = 0;

		void populate(FillBuffer::FillInfo const& fi)
		{
			_buffer = fi.buffer->instance();
			_range = fi.range.value_or(Buffer::Range{});
			if (_range.len == 0)
			{
				_range = Buffer::Range{
					.begin = 0,
					.len = _buffer->createInfo().size,
				};
			}

			resources() += BufferUsage{
				.bari = {_buffer, _range},
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.stage = VK_PIPELINE_STAGE_2_CLEAR_BIT,
				},
				.usage = VK_BUFFER_USAGE_2_TRANSFER_DST_BIT_KHR,
			};
		}

		virtual void clear() override
		{
			ExecutionNode::clear();
			_buffer.reset();
			_range = {};
			_value = 0;
		}

		virtual void execute(ExecutionContext& ctx)
		{
			std::shared_ptr<CommandBuffer> cmd = ctx.getCommandBuffer();
			vkCmdFillBuffer(*cmd, _buffer->handle(), _range.begin, _range.len, _value);
		}
	};

	std::shared_ptr<ExecutionNode> FillBuffer::getExecutionNode(RecordContext& ctx, FillInfo const& fi)
	{
		std::shared_ptr<FillBufferNode> node = _exec_node_cache.getCleanNode<FillBufferNode>([&]() {
			return std::make_shared<FillBufferNode>(FillBufferNode::CI{
				.app = application(),
				.name = name(),
			});
		});
		node->setName(name());
		node->populate(fi);
		return node;
	}

	std::shared_ptr<ExecutionNode> FillBuffer::getExecutionNode(RecordContext& ctx)
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

	struct UpdateBufferNode : public ExecutionNode
	{
		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;
		UpdateBufferNode(CreateInfo const& ci) : 
			ExecutionNode(ExecutionNode::CI{
				.app = ci.app,
				.name = ci.name,
			})
		{}

		ObjectView _src = {};
		std::shared_ptr<BufferInstance> _dst = {};
		size_t _offset = 0;

		void populate(UpdateBuffer::UpdateInfo const& ui)
		{
			_src = ui.src;
			_dst = ui.dst->instance();
			_offset = ui.offset.value_or(0);

			assert(_dst);
			assert(_src.data());

			resources() += BufferUsage{
				.bari = {_dst, Buffer::Range{.begin = _offset, .len = _src.size()}},
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
				.usage = VK_BUFFER_USAGE_2_TRANSFER_DST_BIT_KHR,
			};
		}

		virtual void clear() override
		{
			ExecutionNode::clear();
			_src = {};
			_dst.reset();
			_offset = 0;
		}

		virtual void execute(ExecutionContext& ctx)
		{
			vkCmdUpdateBuffer(*ctx.getCommandBuffer(), *_dst, _offset, _src.size(), _src.data());
		}
	};

	std::shared_ptr<ExecutionNode> UpdateBuffer::getExecutionNode(RecordContext& ctx, UpdateInfo const& ui)
	{
		std::shared_ptr<UpdateBufferNode> node = _exec_node_cache.getCleanNode<UpdateBufferNode>([&]() {
			return std::make_shared<UpdateBufferNode>(UpdateBufferNode::CI{
				.app = application(),
				.name = name(),
			});
		});
		node->setName(name());
		node->populate(ui);
		return node;
	}

	std::shared_ptr<ExecutionNode> UpdateBuffer::getExecutionNode(RecordContext& ctx)
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

	struct UploadBufferNode : public ExecutionNode
	{
		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;
		UploadBufferNode(CreateInfo const& ci) :
			ExecutionNode(ExecutionNode::CI{
				.app = ci.app,
				.name = ci.name,
			})
		{}

		Array<PositionedObjectView> _sources = {};
		std::shared_ptr<BufferInstance> _dst = nullptr;
		bool _use_update = false;
		// optional
		std::shared_ptr<StagingBuffer> _staging_buffer = nullptr;
		Buffer::Range _range; // Merged range of all sources

		// Cached execute() variable
		MyVector<VkBufferCopy2> _regions;

		void populate(RecordContext & ctx, UploadBuffer::UploadInfo const& ui)
		{
			_sources = ui.sources;
			_dst = ui.dst->instance();
			_use_update = [&]() {
				bool res = ui.use_update_buffer_ifp.value_or(true);
				if (res)
				{
					const uint32_t max_size = 65536;
					for (const auto& src : _sources)
					{
						res &= (src.obj.size() <= max_size);
						res &= (src.obj.size() % 4 == 0);
					}
				}
				return res;
			}();

			// first consider .len as .end
			Buffer::Range buffer_range{ .begin = size_t(-1), .len = 0 };
			for (const auto& src : _sources)
			{
				buffer_range.begin = std::min(buffer_range.begin, src.pos);
				buffer_range.len = std::max(buffer_range.len, src.obj.size() + src.pos);
			}
			// now .len is .len
			buffer_range.len = buffer_range.len - buffer_range.begin;
			_range = buffer_range;

			const bool merge_synch = true;
			if (merge_synch)
			{
				resources() += BufferUsage{
					.bari = {_dst, buffer_range},
					.begin_state = {
						.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
						.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
					},
					.usage = VK_BUFFER_USAGE_2_TRANSFER_DST_BIT_KHR,
				};
			}
			else
			{
				for (const auto& src : ui.sources)
				{
					resources() += BufferUsage{
						.bari = {_dst, Buffer::Range{.begin = src.pos, .len = src.obj.size()}},
						.begin_state = {
							.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
							.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
						},
						.usage = VK_BUFFER_USAGE_2_TRANSFER_DST_BIT_KHR,
					};
				}
			}

			if (!_use_update && ctx.stagingPool())
			{
				_staging_buffer = std::make_shared<StagingBuffer>(ctx.stagingPool(), buffer_range.len);
				
				resources() += BufferUsage{
					.bari = {_staging_buffer->buffer(), Buffer::Range{.begin = 0, .len = buffer_range.len}},
					.begin_state = ResourceState2{
						.access = VK_ACCESS_2_HOST_WRITE_BIT,
						.stage = VK_PIPELINE_STAGE_2_HOST_BIT,
					},
					.end_state = ResourceState2{
						.access = VK_ACCESS_2_TRANSFER_READ_BIT,
						.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
					},
					.usage = VK_BUFFER_USAGE_2_TRANSFER_BITS_KHR,
				};
			}
		}

		virtual void clear() override
		{
			ExecutionNode::clear();
			_sources.clear();
			_dst.reset();
			_use_update = false;
			_staging_buffer.reset();
			_range = {};

			_regions.clear();
		}

		virtual void execute(ExecutionContext& ctx)
		{
			if (_sources.empty())	return;
			CommandBuffer& cmd = *ctx.getCommandBuffer();


			if (_use_update)
			{
				for (const auto& src : _sources)
				{
					vkCmdUpdateBuffer(cmd, *_dst, src.pos, src.obj.size(), src.obj.data());
				}
			}
			else // Use Staging Buffer
			{
				std::shared_ptr<StagingBuffer> sb;
				const bool extern_sb = _staging_buffer.operator bool();
				if (extern_sb)
				{
					// Assume externally synched
					sb = _staging_buffer;
				}
				else
				{
					sb = std::make_shared<StagingBuffer>(ctx.stagingPool(), _range.len);
					{
						InlineSynchronizeBuffer(ctx, 
							BufferAndRangeInstance{
								.buffer = sb->buffer(), 
								.range = Buffer::Range{.begin = 0, .len = _range.len}
							},
							ResourceState2{
								.access = VK_ACCESS_2_HOST_WRITE_BIT ,
								.stage = VK_PIPELINE_STAGE_2_HOST_BIT,
							}
						);
					}
				}
				// Copy to Staging Buffer
				{
					sb->buffer()->map();
					for (const auto& src : _sources)
					{
						std::memcpy(static_cast<uint8_t*>(sb->buffer()->data()) + src.pos, src.obj.data(), src.obj.size());
					}
					vmaFlushAllocation(sb->buffer()->allocator(), sb->buffer()->allocation(), 0, _range.len);
					// Flush each subrange individually? probably slow
					sb->buffer()->unMap();
				}

				if (extern_sb)
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
						.size = _range.len,
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
					InlineSynchronizeBuffer(ctx,
						BufferAndRangeInstance{
							.buffer = sb->buffer(),
							.range = Buffer::Range{.begin = 0, .len = _range.len}
						},
						ResourceState2{
							.access = VK_ACCESS_2_TRANSFER_READ_BIT,
							.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
						}
					);
				}

				_regions.resize(_sources.size());
				for (size_t r = 0; r < _regions.size(); ++r)
				{
					_regions[r] = VkBufferCopy2{
						.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
						.pNext = nullptr,
						.srcOffset = _sources[r].pos,
						.dstOffset = _sources[r].pos + _range.begin,
						.size = _sources[r].obj.size(),
					};
				}

				VkCopyBufferInfo2 copy{
					.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
					.pNext = nullptr,
					.srcBuffer = sb->buffer()->handle(),
					.dstBuffer = _dst->handle(),
					.regionCount = _regions.size32(),
					.pRegions = _regions.data(),
				};

				vkCmdCopyBuffer2(cmd, &copy);

				if (!extern_sb)
				{
					ctx.keepAlive(sb);
				}
			}
		}
	};

	std::shared_ptr<ExecutionNode> UploadBuffer::getExecutionNode(RecordContext& ctx, UploadInfo const& ui)
	{
		std::shared_ptr<UploadBufferNode> node = _exec_node_cache.getCleanNode<UploadBufferNode>([&]() {
			return std::make_shared<UploadBufferNode>(UploadBufferNode::CI{
				.app = application(),
				.name = name(),
			});
		});
		node->setName(name());
		node->populate(ctx, ui);
		return node;
	}

	std::shared_ptr<ExecutionNode> UploadBuffer::getExecutionNode(RecordContext& ctx)
	{
		return getExecutionNode(ctx, getDefaultUploadInfo());
	}


	Executable UploadBuffer::with(UploadInfo const& ui)
	{
		return [this, ui](RecordContext & ctx)
		{
			UploadInfo _ui{
				.sources = (!ui.sources.empty()) ? ui.sources : Array<PositionedObjectView>{PositionedObjectView{.obj = _src, .pos = _offset.valueOr(0), }},
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

	struct UploadImageNode : public ExecutionNode
	{
		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;
		UploadImageNode(CreateInfo const& ci) :
			ExecutionNode(ExecutionNode::CI{
				.app = ci.app,
				.name = ci.name,
			})
		{}

		ObjectView _src = {};
		uint32_t _buffer_row_length = 0;
		uint32_t _buffer_image_height = 0;
		std::shared_ptr<ImageViewInstance> _dst = nullptr;
		std::shared_ptr<StagingBuffer> _staging_buffer = nullptr;

		void populate(RecordContext& ctx, UploadImage::UploadInfo const& ui)
		{
			_src = ui.src;
			_buffer_row_length = ui.buffer_row_length;
			_buffer_image_height = ui.buffer_image_height;
			_dst = ui.dst->instance();

			resources() += ImageViewUsage{
				.ivi = _dst,
				.begin_state = {
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
				.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			};

			if (ctx.stagingPool())
			{
				_staging_buffer = std::make_shared<StagingBuffer>(ctx.stagingPool(), _src.size());
				resources() += BufferUsage{
					.bari = {_staging_buffer->buffer(), Buffer::Range{.begin = 0, .len = _src.size()}},
					.begin_state = ResourceState2{
						.access = VK_ACCESS_2_HOST_WRITE_BIT,
						.stage = VK_PIPELINE_STAGE_2_HOST_BIT,
					},
					.end_state = ResourceState2{
						.access = VK_ACCESS_2_TRANSFER_READ_BIT,
						.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
					},
					.usage = VK_BUFFER_USAGE_TRANSFER_BITS,
				};
			}
		}

		virtual void clear()override
		{
			ExecutionNode::clear();

			_src.clear();
			_buffer_row_length = 0;
			_buffer_image_height = 0;
			_dst.reset();
			_staging_buffer.reset();
		}

		virtual void execute(ExecutionContext& ctx)
		{
			CommandBuffer& cmd = *ctx.getCommandBuffer();

			std::shared_ptr<StagingBuffer> sb;
			const bool extern_sb = _staging_buffer.operator bool();
			if (extern_sb)
			{
				// Assume externally synched
				sb = _staging_buffer;
			}
			else
			{
				sb = std::make_shared<StagingBuffer>(ctx.stagingPool(), _src.size());

				InlineSynchronizeBuffer(ctx,
					BufferAndRangeInstance{
						.buffer = sb->buffer(),
						.range = Buffer::Range{.begin = 0, .len = _src.size()}
					},
					ResourceState2{
						.access = VK_ACCESS_2_HOST_WRITE_BIT,
						.stage = VK_PIPELINE_STAGE_2_HOST_BIT,
					}
				);
			}

			// Copy to Staging Buffer
			{
				sb->buffer()->map();
				std::memcpy(sb->buffer()->data(), _src.data(), _src.size());
				vmaFlushAllocation(sb->buffer()->allocator(), sb->buffer()->allocation(), 0, _src.size());
				sb->buffer()->unMap();
			}

			if (extern_sb)
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
					.size = _src.size(),
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
				InlineSynchronizeBuffer(ctx,
					BufferAndRangeInstance{
						.buffer = sb->buffer(),
						.range = Buffer::Range{.begin = 0, .len = _src.size()}
					},
					ResourceState2{
						.access = VK_ACCESS_2_TRANSFER_READ_BIT,
						.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
					}
				);
			}

			VkBufferImageCopy2 region{
				.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
				.pNext = nullptr,
				.bufferOffset = 0,
				.bufferRowLength = _buffer_row_length,   // 0 => tightly packed
				.bufferImageHeight = _buffer_image_height, // 0 => tightly packed
				.imageSubresource = getImageLayersFromRange(_dst->createInfo().subresourceRange),
				.imageOffset = makeZeroOffset3D(),
				.imageExtent = _dst->image()->createInfo().extent,
			};

			VkCopyBufferToImageInfo2 copy{
				.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
				.pNext = nullptr,
				.srcBuffer = sb->buffer()->handle(),
				.dstImage = _dst->image()->handle(),
				.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.regionCount = 1,
				.pRegions = &region,
			};

			vkCmdCopyBufferToImage2(cmd, &copy);

			if (!extern_sb)
			{
				ctx.keepAlive(sb);
			}
		}
	};

	std::shared_ptr<ExecutionNode> UploadImage::getExecutionNode(RecordContext& ctx, UploadInfo const& ui)
	{
		std::shared_ptr<UploadImageNode> node = _exec_node_cache.getCleanNode<UploadImageNode>([&]() {
			return std::make_shared<UploadImageNode>(UploadImageNode::CI{
				.app = application(),
				.name = name(),
			});	
		});

		node->setName(name());
		node->populate(ctx, ui);
		return node;
	}

	std::shared_ptr<ExecutionNode> UploadImage::getExecutionNode(RecordContext& ctx)
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

	struct UploadResourcesNode : public ExecutionNode
	{
		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;
		UploadResourcesNode(CreateInfo const& ci) :
			ExecutionNode(ExecutionNode::CI{
				.app = ci.app,
				.name = ci.name,
			})
		{}

		struct BufferUploadExtraInfo
		{
			bool use_update = false;
			Buffer::Range range = {};
		};

		// Assuming no aliasing between resources
		ResourcesToUpload _upload_list = {};
		std::shared_ptr<StagingBuffer> _staging_buffer = nullptr;
		MyVector<BufferUploadExtraInfo> _extra_buffer_info;

		void populate(RecordContext& ctx, UploadResources::UploadInfo const& ui)
		{
			_upload_list = ui.upload_list;
			
			_extra_buffer_info.resize(_upload_list.buffers.size());

			for (size_t i = 0; i < _upload_list.buffers.size(); ++i)
			{
				const auto& buffer_upload = _upload_list.buffers[i];
				assert(buffer_upload.sources);

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

				_extra_buffer_info[i] = {
					.use_update = use_update,
					.range = buffer_range,
				};

				if (merge_synch)
				{
					resources() += BufferUsage{
						.bari = {buffer_upload.dst, buffer_range},
						.begin_state = {
							.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
							.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
						},
						.usage = VK_BUFFER_USAGE_2_TRANSFER_DST_BIT_KHR,
					};
				}
				else
				{
					for (const auto& src : buffer_upload.sources)
					{
						resources() += BufferUsage{
							.bari = {buffer_upload.dst, Buffer::Range{.begin = src.pos, .len = src.obj.size()}},
							.begin_state = {
								.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
								.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
							},
							.usage = VK_BUFFER_USAGE_2_TRANSFER_DST_BIT_KHR,
						};
					}
				}
			}
			for (const auto& image_upload : _upload_list.images)
			{
				resources() += ImageViewUsage{
					.ivi = image_upload.dst,
					.begin_state = {
						.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
						.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
					},
					.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				};
			}
		}

		virtual void clear() override
		{
			ExecutionNode::clear();
			_upload_list.clear();
			_staging_buffer.reset();
			_extra_buffer_info.clear();
		}

		virtual void execute(ExecutionContext& ctx)
		{
			const ResourcesToUpload& resources = _upload_list;

			UploadImageNode image_uploader(UploadImageNode::CI{
				.app = application(),
				.name = name() + ".ImageUploader",
			});

			UploadBufferNode buffer_uploader(UploadBufferNode::CI{
				.app = application(),
				.name = name() + ".BufferUploader",
			});

			std::shared_ptr<CallbackHolder> completion_callbacks;

			// TODO use a single staging buffer 
			// TODO rewrite this function someday (not rely on other commands to be more efficient)

			for (size_t i = 0; i < resources.buffers.size(); ++i)
			{
				auto& buffer_upload = resources.buffers[i];

				buffer_uploader.clear();
				buffer_uploader._sources = buffer_upload.sources,
				buffer_uploader._dst = buffer_upload.dst;
				buffer_uploader._use_update = _extra_buffer_info[i].use_update;
				buffer_uploader._staging_buffer = nullptr;
				buffer_uploader._range = _extra_buffer_info[i].range;

				buffer_uploader.execute(ctx);

				if (buffer_upload.completion_callback)
				{
					if (!completion_callbacks) completion_callbacks = std::make_shared<CallbackHolder>();
					completion_callbacks->callbacks.push_back(buffer_upload.completion_callback);
				}
			}

			for (auto& image_upload : resources.images)
			{
				image_uploader.clear();
				image_uploader._src = image_upload.src;
				image_uploader._buffer_row_length = image_upload.buffer_row_length;
				image_uploader._buffer_image_height = image_upload.buffer_image_height;
				image_uploader._dst = image_upload.dst;
				image_uploader.execute(ctx);

				if (image_upload.completion_callback)
				{
					if (!completion_callbacks) completion_callbacks = std::make_shared<CallbackHolder>();
					completion_callbacks->callbacks.push_back(image_upload.completion_callback);
				}
			}

			if (completion_callbacks.operator bool() && !completion_callbacks->callbacks.empty())
			{
				ctx.keepAlive(completion_callbacks);
			}
		}
	};

	std::shared_ptr<ExecutionNode> UploadResources::getExecutionNode(RecordContext& ctx, UploadInfo const& ui)
	{
		std::shared_ptr<UploadResourcesNode> node = _exec_node_cache.getCleanNode<UploadResourcesNode>([&]() {
			return std::make_shared<UploadResourcesNode>(UploadResourcesNode::CI{
				.app = application(),
				.name = name(),
			});
		});
		node->setName(name());
		node->populate(ctx, ui);
		return node;
	}

	std::shared_ptr<ExecutionNode> UploadResources::getExecutionNode(RecordContext& ctx)
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