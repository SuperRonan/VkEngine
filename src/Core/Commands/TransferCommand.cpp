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
		VkImageLayout _src_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageLayout _dst_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		Array<VkImageCopy2> _regions = {};

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
			// TODO move to CopyImage2
			std::shared_ptr<CommandBuffer> cmd = ctx.getCommandBuffer();

			const VkImageCopy2* regions = _regions.data();
			uint32_t n_regions = _regions.size32();
			VkImageCopy2 _region;
			if (_regions.empty())
			{
				_region = VkImageCopy2{
					.sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
					.pNext = nullptr,
					.srcSubresource = getImageLayersFromRange(_src->createInfo().subresourceRange),
					.srcOffset = makeZeroOffset3D(),
					.dstSubresource = getImageLayersFromRange(_dst->createInfo().subresourceRange),
					.dstOffset = makeZeroOffset3D(),
					.extent = _dst->image()->createInfo().extent,
				};
				regions = &_region;
				n_regions = 1;
			}

			const VkCopyImageInfo2 info{
				.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2,
				.pNext = nullptr,
				.srcImage = _src->image()->handle(),
				.srcImageLayout = _src_layout,
				.dstImage = _dst->image()->handle(),
				.dstImageLayout = _dst_layout,
				.regionCount = n_regions,
				.pRegions = regions,
			};
			vkCmdCopyImage2(*cmd, &info);
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
		node->_src_layout = application()->options().getLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		node->resources() += ImageViewUsage{
			.ivi = node->_src,
			.begin_state = ResourceState2{
				.access = VK_ACCESS_2_TRANSFER_READ_BIT,
				.layout = node->_src_layout,
				.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
			},
			.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		};

		node->_dst = ci.dst->instance();
		node->_dst_layout = application()->options().getLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		node->resources() += ImageViewUsage{
			.ivi = node->_dst,
			.begin_state = ResourceState2{
				.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.layout = node->_dst_layout,
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

		BufferAndRangeInstance _src = {};
		std::shared_ptr<ImageViewInstance> _dst = nullptr;
		VkImageLayout _dst_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		Array<VkBufferImageCopy2> _regions = {};
		uint32_t _default_buffer_row_length = 0;
		uint32_t _default_buffer_image_height = 0;

		virtual void clear() override
		{
			ExecutionNode::clear();
			_src = {};
			_dst.reset();
			_regions.clear();
			_default_buffer_row_length = 0;
			_default_buffer_image_height = 0;
		}

		virtual void execute(ExecutionContext& ctx) override
		{
			std::shared_ptr<CommandBuffer> cmd = ctx.getCommandBuffer();

			uint32_t n_regions = _regions.size32();
			const VkBufferImageCopy2* p_regions = _regions.data();
			VkBufferImageCopy2 _reg;
			if (n_regions == 0)
			{
				const VkExtent3D extent = _dst->image()->createInfo().extent;
				_reg = VkBufferImageCopy2{
					.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
					.pNext = nullptr,
					.bufferOffset = _src.range.begin,
					.bufferRowLength = _default_buffer_row_length,   // 0 => tightly packed
					.bufferImageHeight = _default_buffer_image_height, // 0 => tightly packed
					.imageSubresource = getImageLayersFromRange(_dst->createInfo().subresourceRange),
					.imageOffset = makeZeroOffset3D(),
					.imageExtent = extent,
				};
				p_regions = &_reg;
				n_regions = 1;
			}

			const VkCopyBufferToImageInfo2 info{
				.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
				.pNext = nullptr,
				.srcBuffer = _src.buffer->handle(),
				.dstImage = _dst->image()->handle(),
				.dstImageLayout = _dst_layout,
				.regionCount = n_regions,
				.pRegions = p_regions,
			};

			vkCmdCopyBufferToImage2(*cmd, &info);
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

		node->_src = ci.src.getInstance();
		node->_dst = ci.dst->instance();
		node->_dst_layout = application()->options().getLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		node->_regions = ci.regions;
		node->_default_buffer_row_length = ci.default_buffer_row_length;
		node->_default_buffer_image_height = ci.default_buffer_image_height;

		node->resources() += BufferUsage{
			.bari = node->_src,
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
				.layout = node->_dst_layout,
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
				.dst = info.dst ? info.dst : _dst,
				.regions = info.regions.empty() ? _regions : info.regions,
				.default_buffer_row_length = info.default_buffer_row_length,
				.default_buffer_image_height = info.default_buffer_image_height,
			};
			return getExecutionNode(context, cinfo);
		};
	}





	CopyImageToBuffer::CopyImageToBuffer(CreateInfo const& ci) :
		TransferCommand(ci.app, ci.name),
		_src(ci.src),
		_dst(ci.dst),
		_regions(ci.regions)
	{}

	struct CopyImageToBufferNode : public ExecutionNode
	{
		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;
		CopyImageToBufferNode(CreateInfo const& ci) :
			ExecutionNode(ExecutionNode::CI{
				.app = ci.app,
				.name = ci.name
				})
		{}

		std::shared_ptr<ImageViewInstance> _src = nullptr;
		BufferAndRangeInstance _dst = {};
		VkImageLayout _src_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		Array<VkBufferImageCopy2> _regions = {};
		uint32_t _default_buffer_row_length = 0;
		uint32_t _default_buffer_image_height = 0;

		virtual void clear() override
		{
			ExecutionNode::clear();
			_src.reset();
			_dst = {};
			_regions.clear();
			_default_buffer_row_length = 0;
			_default_buffer_image_height = 0;
		}

		virtual void execute(ExecutionContext& ctx) override
		{
			std::shared_ptr<CommandBuffer> cmd = ctx.getCommandBuffer();

			uint32_t n_regions = _regions.size32();
			const VkBufferImageCopy2* p_regions = _regions.data();
			VkBufferImageCopy2 _reg;
			if (n_regions == 0)
			{
				const VkExtent3D extent = _src->image()->createInfo().extent;
				_reg = VkBufferImageCopy2{
					.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
					.pNext = nullptr,
					.bufferOffset = _dst.range.begin,
					.bufferRowLength = _default_buffer_row_length,   // 0 => tightly packed
					.bufferImageHeight = _default_buffer_image_height, // 0 => tightly packed
					.imageSubresource = getImageLayersFromRange(_src->createInfo().subresourceRange),
					.imageOffset = makeZeroOffset3D(),
					.imageExtent = extent,
				};
				p_regions = &_reg;
				n_regions = 1;
			}

			const VkCopyImageToBufferInfo2 info{
				.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2,
				.pNext = nullptr,
				.srcImage = _src->image()->handle(),
				.srcImageLayout = _src_layout,
				.dstBuffer = _dst.buffer->handle(),
				.regionCount = n_regions,
				.pRegions = p_regions,
			};

			vkCmdCopyImageToBuffer2(*cmd, &info);
		}
	};

	std::shared_ptr<ExecutionNode> CopyImageToBuffer::getExecutionNode(RecordContext& ctx, CopyInfo const& ci)
	{
		std::shared_ptr<CopyImageToBufferNode> node = _exec_node_cache.getCleanNode<CopyImageToBufferNode>([&]() {
			return std::make_shared<CopyImageToBufferNode>(CopyImageToBufferNode::CI{
				.app = application(),
				.name = name(),
				});
			});
		node->setName(name());

		node->_src = ci.src->instance();
		node->_dst = ci.dst.getInstance();
		node->_src_layout = application()->options().getLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		node->_regions = ci.regions;
		node->_default_buffer_row_length = ci.default_buffer_row_length;
		node->_default_buffer_image_height = ci.default_buffer_image_height;

		node->resources() += ImageViewUsage{
			.ivi = node->_src,
			.begin_state = ResourceState2{
				.access = VK_ACCESS_2_TRANSFER_READ_BIT,
				.layout = node->_src_layout,
				.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
			},
			.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		};
		
		node->resources() += BufferUsage{
			.bari = node->_dst,
			.begin_state = ResourceState2{
				.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
			},
			.usage = VK_BUFFER_USAGE_2_TRANSFER_DST_BIT_KHR,
		};

		return node;
	}

	std::shared_ptr<ExecutionNode> CopyImageToBuffer::getExecutionNode(RecordContext& ctx)
	{
		return getExecutionNode(ctx, getDefaultCopyInfo());
	}

	Executable CopyImageToBuffer::with(CopyInfo const& info)
	{
		return [this, info](RecordContext& context)
			{
				const CopyInfo cinfo{
					.src = info.src ? info.src : _src,
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

		void populate(CopyBuffer::CopyInfoInstance const& ci)
		{
			_src = ci.src;
			_dst = ci.dst;
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

	std::shared_ptr<ExecutionNode> CopyBuffer::getExecutionNode(RecordContext& ctx, CopyInfoInstance const& ci)
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
			CopyInfoInstance ci{
				.src = cinfo.src ? cinfo.src->instance() : _src->instance(),
				.dst = cinfo.dst ? cinfo.dst->instance() : _dst->instance(),
				.regions = cinfo.regions,
			};
			return getExecutionNode(ctx, ci);
		};
	}

	Executable CopyBuffer::with(CopyInfoInstance const& cii)
	{
		return [this, cii](RecordContext& ctx)
		{
			return getExecutionNode(ctx, cii);
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
					.stage = VK_PIPELINE_STAGE_2_CLEAR_BIT,
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
		_use_update_buffer_ifp(ci.use_update_buffer_ifp),
		_staging_pool(ci.staging_pool)
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
		std::shared_ptr<PooledBuffer> _staging_buffer = nullptr;
		Buffer::Range _range; // Merged range of all sources

		// Cached execute() variable
		MyVector<VkBufferCopy2> _regions;

		void populate(RecordContext & ctx, UploadBuffer::UploadInfo const& ui, BufferPool * pool)
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

			const VkPipelineStageFlags2 stage = _use_update ? VK_PIPELINE_STAGE_2_CLEAR_BIT : VK_PIPELINE_STAGE_2_COPY_BIT;

			const bool merge_synch = true;
			if (merge_synch)
			{
				resources() += BufferUsage{
					.bari = {_dst, buffer_range},
					.begin_state = {
						.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
						.stage = stage,
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
							.stage = stage,
						},
						.usage = VK_BUFFER_USAGE_2_TRANSFER_DST_BIT_KHR,
					};
				}
			}

			if (!_use_update)
			{
				assert(pool);
				_staging_buffer = std::make_shared<PooledBuffer>(pool, buffer_range.len);

				// The staging buffer synch is somewhat "reversed"
				// The end stage is set	to host write for the next usage of the staging buffer
				resources() += BufferUsage{
					.bari = {_staging_buffer->buffer(), Buffer::Range{.begin = 0, .len = buffer_range.len}},
					.begin_state = ResourceState2{
						.access = VK_ACCESS_2_TRANSFER_READ_BIT,
						.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
					},
					.end_state = ResourceState2{
						.access = VK_ACCESS_2_HOST_WRITE_BIT,
						.stage = VK_PIPELINE_STAGE_2_HOST_BIT,
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
				std::shared_ptr<PooledBuffer> & sb = _staging_buffer;
				
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

				// Synch staging buffer for next usage
				{
					// Assume externally .end_state
					VkBufferMemoryBarrier2 sb_barrier{
						.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
						.pNext = nullptr,
						.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
						.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
						.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
						.dstAccessMask = VK_ACCESS_2_HOST_WRITE_BIT,
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
		BufferPool * pool = ui.staging_pool ? ui.staging_pool.get() : _staging_pool.get();
		node->populate(ctx, ui, pool);
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
		_dst(ci.dst),
		_staging_pool(ci.staging_pool)
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
		VkImageLayout _dst_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		std::shared_ptr<PooledBuffer> _staging_buffer = nullptr;

		void populate(RecordContext& ctx, UploadImage::UploadInfo const& ui, BufferPool * pool)
		{
			_src = ui.src;
			_buffer_row_length = ui.buffer_row_length;
			_buffer_image_height = ui.buffer_image_height;
			_dst = ui.dst->instance();
			_dst_layout = application()->options().getLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT);

			resources() += ImageViewUsage{
				.ivi = _dst,
				.begin_state = {
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.layout = _dst_layout,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
				.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			};


			{
				_staging_buffer = std::make_shared<PooledBuffer>(pool, _src.size());
				resources() += BufferUsage{
					.bari = {_staging_buffer->buffer(), Buffer::Range{.begin = 0, .len = _src.size()}},
					.begin_state = ResourceState2{
						.access = VK_ACCESS_2_TRANSFER_READ_BIT,
						.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
					},
					.end_state = ResourceState2{
						.access = VK_ACCESS_2_HOST_WRITE_BIT,
						.stage = VK_PIPELINE_STAGE_2_HOST_BIT,
					},
					.usage = VK_BUFFER_USAGE_2_TRANSFER_BITS_KHR,
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

			std::shared_ptr<PooledBuffer> & sb = _staging_buffer;

			// Copy to Staging Buffer
			{
				sb->buffer()->map();
				std::memcpy(sb->buffer()->data(), _src.data(), _src.size());
				vmaFlushAllocation(sb->buffer()->allocator(), sb->buffer()->allocation(), 0, _src.size());
				sb->buffer()->unMap();
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
				.dstImageLayout = _dst_layout,
				.regionCount = 1,
				.pRegions = &region,
			};

			vkCmdCopyBufferToImage2(cmd, &copy);

			{
				// Assume externally .end_state
				VkBufferMemoryBarrier2 sb_barrier{
					.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
					.pNext = nullptr,
					.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
						.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
						.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
						.dstAccessMask = VK_ACCESS_2_HOST_WRITE_BIT,
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
		BufferPool* pool = ui.staging_pool ? ui.staging_pool.get() : _staging_pool.get();
		node->populate(ctx, ui, pool);
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
		_holder(ci.holder),
		_staging_pool(ci.staging_pool)
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
			size_t staging_offset = 0;
		};

		struct ImageUploadExtraInfo
		{
			size_t staging_offset = 0;
		};

		// Assuming no aliasing between resources
		ResourcesToUpload _upload_list = {};
		std::shared_ptr<PooledBuffer> _staging_buffer = nullptr;
		MyVector<BufferUploadExtraInfo> _extra_buffer_info;
		MyVector<ImageUploadExtraInfo> _extra_image_info;
		VkImageLayout _dst_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		size_t _total_staging_size = 0;

		void populate(RecordContext& ctx, UploadResources::UploadInfo const& ui, BufferPool * pool)
		{
			_upload_list = ui.upload_list;
			
			_extra_buffer_info.resize(_upload_list.buffers.size());
			_extra_image_info.resize(_upload_list.images.size());

			_dst_layout = application()->options().getLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT);

			size_t staging_offset = 0;

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
				const VkPipelineStageFlags2 stage = use_update ? VK_PIPELINE_STAGE_2_CLEAR_BIT : VK_PIPELINE_STAGE_2_COPY_BIT;

				// TODO check for range intersection

				// first consider .len as .end
				Buffer::Range buffer_range{ .begin = size_t(-1), .len = 0 };
				size_t total_upload_size = 0;
				for (const auto& src : buffer_upload.sources)
				{
					buffer_range.begin = std::min(buffer_range.begin, src.pos);
					buffer_range.len = std::max(buffer_range.len, src.obj.size() + src.pos);
					total_upload_size += src.obj.size();
				}
				// now .len is .len
				buffer_range.len = buffer_range.len - buffer_range.begin;
				const bool merge_synch = true;

				_extra_buffer_info[i] = {
					.use_update = use_update,
					.range = buffer_range,
					.staging_offset = staging_offset,
				};

				if (!use_update)
				{
					staging_offset += std::alignUp(total_upload_size, size_t(4));
				}

				if (merge_synch)
				{
					resources() += BufferUsage{
						.bari = {buffer_upload.dst, buffer_range},
						.begin_state = {
							.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
							.stage = stage,
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
								.stage = stage,
							},
							.usage = VK_BUFFER_USAGE_2_TRANSFER_DST_BIT_KHR,
						};
					}
				}
			}
			// TODO
			const size_t image_align = 128;
			staging_offset = std::alignUp(staging_offset, image_align);
			for (size_t i = 0; i < _upload_list.images.size(); ++i)
			{
				const auto& image_upload = _upload_list.images[i];

				_extra_image_info[i].staging_offset = staging_offset;
				staging_offset += std::alignUp(image_upload.src.size(), image_align);

				resources() += ImageViewUsage{
					.ivi = image_upload.dst,
					.begin_state = {
						.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
						.layout = _dst_layout,
						.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
					},
					.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				};
			}

			_total_staging_size = staging_offset;
			if (_staging_buffer)
			{
				if (_staging_buffer->buffer()->createInfo().size < _total_staging_size)
				{
					_staging_buffer = nullptr;
				}
			}
			if (!_staging_buffer && _total_staging_size > 0)
			{
				_staging_buffer = std::make_shared<PooledBuffer>(pool, _total_staging_size);
			}
			if (_staging_buffer)
			{
				resources() += BufferUsage{
					.bari = BufferAndRangeInstance{.buffer = _staging_buffer->buffer(), .range = _staging_buffer->buffer()->fullRange(),},
					.begin_state = ResourceState2{
						.access = VK_ACCESS_2_TRANSFER_READ_BIT,
						.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
					},
					.end_state = ResourceState2{
						.access = VK_ACCESS_2_HOST_WRITE_BIT,
						.stage = VK_PIPELINE_STAGE_2_HOST_BIT,
					},
					.usage = VK_BUFFER_USAGE_2_TRANSFER_BITS_KHR,
				};
			}
		}

		virtual void clear() override
		{
			ExecutionNode::clear();
			_upload_list.clear();
			_staging_buffer.reset();
			_extra_buffer_info.clear();
			_extra_image_info.clear();
			_buffer_regions.clear();
			_total_staging_size = 0;
		}


		MyVector<VkBufferCopy2> _buffer_regions;

		virtual void execute(ExecutionContext& ctx)
		{
			const ResourcesToUpload& resources = _upload_list;
			VkCommandBuffer cmd = ctx.getCommandBuffer()->handle();

			uint8_t* data = nullptr;
			if (_staging_buffer)
			{
				_buffer_regions.clear();
				data = static_cast<uint8_t*>(_staging_buffer->buffer()->map());
			}

			std::shared_ptr<CallbackHolder> completion_callbacks;
			const auto pushCallbackIFN = [&](CompletionCallback const& cb)
			{
				if (cb)
				{
					if (!completion_callbacks)
					{
						completion_callbacks = std::make_shared<CallbackHolder>();
					}
					completion_callbacks->callbacks.push_back(cb);
				}
			};

			for (size_t i = 0; i < resources.buffers.size(); ++i)
			{
				if (_extra_buffer_info[i].use_update)
				{
					for (size_t j = 0; j < resources.buffers[i].sources.size(); ++j)
					{
						vkCmdUpdateBuffer(cmd, 
							resources.buffers[i].dst->handle(), 
							resources.buffers[i].sources[j].pos, 
							resources.buffers[i].sources[j].obj.size(), 
							resources.buffers[i].sources[j].obj.data()
						);
					}
				}
				else
				{
					assert(data);
					size_t s = 0;
					_buffer_regions.resize(resources.buffers[i].sources.size());
					for (size_t j = 0; j < resources.buffers[i].sources.size(); ++j)
					{
						const size_t sb_offset = _extra_buffer_info[i].staging_offset + s;
						uint8_t * dst = data + sb_offset;
						const void * src = resources.buffers[i].sources[j].obj.data();
						const size_t size = resources.buffers[i].sources[j].obj.size();
						std::memcpy(dst, src, size);
						_buffer_regions[j] = VkBufferCopy2{
							.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
							.pNext = nullptr,
							.srcOffset = sb_offset,
							.dstOffset = resources.buffers[i].sources[j].pos,
							.size = size,
						};
						s += resources.buffers[i].sources[j].obj.size();

					}
					const VkCopyBufferInfo2 info{
						.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
						.pNext = nullptr,
						.srcBuffer = _staging_buffer->buffer()->handle(),
						.dstBuffer = resources.buffers[i].dst->handle(),
						.regionCount = _buffer_regions.size32(),
						.pRegions = _buffer_regions.data(),
					};

					vkCmdCopyBuffer2(cmd, &info);
				}
				pushCallbackIFN(resources.buffers[i].completion_callback);
			}
			for (size_t i = 0; i < resources.images.size(); ++i)
			{
				std::memcpy(data + _extra_image_info[i].staging_offset, resources.images[i].src.data(), resources.images[i].src.size());
				const VkBufferImageCopy2 region{
					.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
					.pNext = nullptr,
					.bufferOffset = _extra_image_info[i].staging_offset,
					.bufferRowLength = resources.images[i].buffer_row_length,
					.bufferImageHeight = resources.images[i].buffer_image_height,
					.imageSubresource = getImageLayersFromRange(resources.images[i].dst->createInfo().subresourceRange), // Copy to base mip only
					.imageOffset = makeZeroOffset3D(),
					.imageExtent = resources.images[i].dst->image()->createInfo().extent,
				};
				const VkCopyBufferToImageInfo2 info{
					.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
					.pNext = nullptr,
					.srcBuffer = _staging_buffer->buffer()->handle(),
					.dstImage = resources.images[i].dst->image()->handle(),
					.dstImageLayout = _dst_layout,
					.regionCount = 1,
					.pRegions = &region,
				};
				vkCmdCopyBufferToImage2(cmd, &info);
				pushCallbackIFN(resources.images[i].completion_callback);
			}
			if (_staging_buffer)
			{
				_staging_buffer->buffer()->unMap();
				_staging_buffer->buffer()->flush();
				{
					// Assume externally .end_state
					VkBufferMemoryBarrier2 sb_barrier{
						.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
						.pNext = nullptr,
						.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
							.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
							.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
							.dstAccessMask = VK_ACCESS_2_HOST_WRITE_BIT,
						.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.buffer = _staging_buffer->buffer()->handle(),
						.offset = 0,
						.size = _staging_buffer->buffer()->createInfo().size,
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
			}

			if (completion_callbacks.operator bool() && !completion_callbacks->callbacks.empty())
			{
				ctx.keepAlive(completion_callbacks);
			}
			ctx.keepAlive(_staging_buffer);
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
		BufferPool* pool = ui.staging_pool ? ui.staging_pool.get() : _staging_pool.get();
		node->populate(ctx, ui, pool);
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








	DownloadBuffer::DownloadBuffer(CreateInfo const& ci) :
		TransferCommand(ci.app, ci.name),
		_src(ci.src),
		_dst(ci.dst),
		_staging_pool(ci.staging_pool)
	{

	}

	struct DownloadBufferNode : public ExecutionNode
	{
		BufferAndRangeInstance _src = {};
		void * _dst = nullptr;
		DownloadCallback _completion_callback = {};
		std::shared_ptr<PooledBuffer> _staging_buffer = {};
		
		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;

		DownloadBufferNode(CreateInfo const& ci):
			ExecutionNode(ExecutionNode::CI{
				.app = ci.app,
				.name = ci.name,
			})
		{}

		virtual void clear() override
		{
			ExecutionNode::clear();
			_src = {};
			_dst = nullptr;
			_completion_callback = {};
			_staging_buffer.reset();
		}

		void populate(RecordContext& ctx, DownloadBuffer::DownloadInfo const& di, BufferPool* pool)
		{
			_src = di.src.getInstance();
			_dst = di.dst;
			_completion_callback = di.completion_callback;

			_staging_buffer = std::make_shared<PooledBuffer>(pool, _src.size());

			resources() += BufferUsage{
				.bari = _src,
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_READ_BIT,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
			};

			resources() += BufferUsage{
				.bari = _staging_buffer->bufferSegment(),
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
				.end_state = ResourceState2{
					.access = VK_ACCESS_2_HOST_READ_BIT,
					.stage = VK_PIPELINE_STAGE_2_HOST_BIT,
				},
			};
		}

		virtual void execute(ExecutionContext& ctx) override
		{
			VkCommandBuffer cmd = ctx.getCommandBuffer()->handle();
			VkBufferCopy2 region{
				.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
				.pNext = nullptr,
				.srcOffset = _src.range.begin,
				.dstOffset = 0,
				.size = _src.size(),
			};

			VkCopyBufferInfo2 info{
				.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
				.pNext = nullptr,
				.srcBuffer = _src.buffer->handle(),
				.dstBuffer = _staging_buffer->buffer()->handle(),
				.regionCount = 1,
				.pRegions = &region,
			};

			vkCmdCopyBuffer2(cmd, &info);

			{
				// Assume externally .end_state
				VkBufferMemoryBarrier2 sb_barrier{
					.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
					.pNext = nullptr,
					.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
					.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
					.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.buffer = _staging_buffer->buffer()->handle(),
					.offset = 0,
					.size = _staging_buffer->buffer()->createInfo().size,
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

			ctx.keepAlive(_staging_buffer);

			if (_dst || _completion_callback)
			{
				void * dst = _dst;
				DownloadCallback download_callback = _completion_callback;
				std::shared_ptr<PooledBuffer> sb = _staging_buffer;
				size_t size = _src.size();
				CompletionCallback cb = [dst, download_callback, sb, size](int int_res)
				{
					VkResult res = static_cast<VkResult>(int_res);
					if (res == VK_SUCCESS)
					{
						if (dst)
						{
							sb->buffer()->flush();
							void * sb_data = sb->buffer()->map();
							assert(sb_data != nullptr);
							std::memcpy(dst, sb_data, size);
							sb->buffer()->unMap();
						}
					}
					if (download_callback)
					{
						download_callback(int_res, sb);
					}
				};
				ctx.addCompletionCallback(cb);
			}
			else
			{
				// ???
			}
		}
	};

	std::shared_ptr<ExecutionNode> DownloadBuffer::getExecutionNode(RecordContext& ctx, DownloadInfo const& di)
	{
		std::shared_ptr<DownloadBufferNode> node = _exec_node_cache.getCleanNode<DownloadBufferNode>([&]() {
			return std::make_shared<DownloadBufferNode>(DownloadBufferNode::CI{
				.app = application(),
				.name = name(),
			});
		});

		node->setName(name());
		BufferPool * pool = di.staging_pool ? di.staging_pool.get() : _staging_pool.get();
		node->populate(ctx, di, pool);
		return node;
	}

	std::shared_ptr<ExecutionNode> DownloadBuffer::getExecutionNode(RecordContext& ctx)
	{
		DownloadInfo di{
			.src = _src,
			.dst = _dst,
			.staging_pool = nullptr,
			.completion_callback = nullptr,
		};
		return getExecutionNode(ctx, di);
	}

	Executable DownloadBuffer::with(DownloadInfo const& di)
	{
		DownloadInfo _di{
			.src = di.src ? di.src : _src,
			.dst = di.dst ? di.dst : _dst,
			.staging_pool = di.staging_pool,
			.completion_callback = di.completion_callback,
		};
		return [this, _di](RecordContext& ctx)
		{
			return getExecutionNode(ctx, _di);
		};
	}














	DownloadImage::DownloadImage(CreateInfo const& ci) :
		TransferCommand(ci.app, ci.name),
		_src(ci.src),
		_dst(ci.dst),
		_size(ci.size),
		_staging_pool(ci.staging_pool)
	{

	}

	struct DownloadImageNode : public ExecutionNode
	{
		std::shared_ptr<ImageViewInstance> _src = nullptr;
		void* _dst = nullptr;
		size_t _size = 0;
		uint32_t _buffer_row_length = 0;
		uint32_t _buffer_image_height = 0;
		DownloadCallback _completion_callback = {};
		std::shared_ptr<PooledBuffer> _staging_buffer = {};

		VkImageLayout _src_layout = VK_IMAGE_LAYOUT_UNDEFINED;

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;

		DownloadImageNode(CreateInfo const& ci) :
			ExecutionNode(ExecutionNode::CI{
				.app = ci.app,
				.name = ci.name,
			})
		{}

		virtual void clear() override
		{
			ExecutionNode::clear();
			_src.reset();
			_dst = nullptr;
			_size = 0;
			_buffer_row_length = 0;
			_buffer_image_height = 0;
			_completion_callback = {};
			_staging_buffer.reset();
			_src_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		}

		void populate(RecordContext& ctx, DownloadImage::DownloadInfo const& di, BufferPool* pool)
		{
			_src = di.src->instance();
			_dst = di.dst;
			_completion_callback = di.completion_callback;

			_size = di.size;
			_buffer_row_length = di.default_buffer_row_length;
			_buffer_image_height = di.default_buffer_image_height;

			_staging_buffer = std::make_shared<PooledBuffer>(pool, _size);

			_src_layout = application()->options().getLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

			resources() += ImageViewUsage{
				.ivi = _src,
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_READ_BIT,
					.layout = _src_layout,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
			};

			resources() += BufferUsage{
				.bari = _staging_buffer->bufferSegment(),
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.stage = VK_PIPELINE_STAGE_2_COPY_BIT,
				},
				.end_state = ResourceState2{
					.access = VK_ACCESS_2_HOST_READ_BIT,
					.stage = VK_PIPELINE_STAGE_2_HOST_BIT,
				},
			};
		}

		virtual void execute(ExecutionContext& ctx) override
		{
			VkCommandBuffer cmd = ctx.getCommandBuffer()->handle();

			VkBufferImageCopy2 region{
				.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
				.pNext = nullptr,
				.bufferOffset = 0,
				.bufferRowLength = _buffer_row_length,
				.bufferImageHeight = _buffer_image_height,
				.imageSubresource = getImageLayersFromRange(_src->createInfo().subresourceRange),
				.imageOffset = makeZeroOffset3D(),
				.imageExtent = _src->image()->createInfo().extent,
			};

			VkCopyImageToBufferInfo2 info{
				.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2,
				.pNext = nullptr,
				.srcImage = _src->image()->handle(),
				.srcImageLayout = _src_layout,
				.dstBuffer = _staging_buffer->buffer()->handle(),
				.regionCount = 1,
				.pRegions = &region,
			};

			vkCmdCopyImageToBuffer2(cmd, &info);

			{
				// Assume externally .end_state
				VkBufferMemoryBarrier2 sb_barrier{
					.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
					.pNext = nullptr,
					.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
					.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
					.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.buffer = _staging_buffer->buffer()->handle(),
					.offset = 0,
					.size = _staging_buffer->buffer()->createInfo().size,
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

			ctx.keepAlive(_staging_buffer);

			if (_dst || _completion_callback)
			{
				void* dst = _dst;
				DownloadCallback download_callback = _completion_callback;
				std::shared_ptr<PooledBuffer> sb = _staging_buffer;
				size_t size = _size;
				CompletionCallback cb = [dst, download_callback, sb, size](int int_res)
					{
						VkResult res = static_cast<VkResult>(int_res);
						if (res == VK_SUCCESS)
						{
							if (dst)
							{
								sb->buffer()->flush();
								void* sb_data = sb->buffer()->map();
								assert(sb_data != nullptr);
								std::memcpy(dst, sb_data, size);
								sb->buffer()->unMap();
							}
						}
						if (download_callback)
						{
							download_callback(int_res, sb);
						}
					};
				ctx.addCompletionCallback(cb);
			}
			else
			{
				// ???
			}
		}
	};

	std::shared_ptr<ExecutionNode> DownloadImage::getExecutionNode(RecordContext& ctx, DownloadInfo const& di)
	{
		std::shared_ptr<DownloadImageNode> node = _exec_node_cache.getCleanNode<DownloadImageNode>([&]() {
			return std::make_shared<DownloadImageNode>(DownloadImageNode::CI{
				.app = application(),
				.name = name(),
				});
			});

		node->setName(name());
		BufferPool* pool = di.staging_pool ? di.staging_pool.get() : _staging_pool.get();
		node->populate(ctx, di, pool);
		return node;
	}

	std::shared_ptr<ExecutionNode> DownloadImage::getExecutionNode(RecordContext& ctx)
	{
		DownloadInfo di{
			.src = _src,
			.dst = _dst,
			.size = _size,
			.default_buffer_row_length = 0,
			.default_buffer_image_height = 0,
			.staging_pool = nullptr,
			.completion_callback = nullptr,
		};
		return getExecutionNode(ctx, di);
	}

	Executable DownloadImage::with(DownloadInfo const& di)
	{
		DownloadInfo _di{
			.src = di.src ? di.src : _src,
			.dst = di.dst ? di.dst : _dst,
			.staging_pool = di.staging_pool,
			.completion_callback = di.completion_callback,
		};
		return [this, _di](RecordContext& ctx)
		{
			return getExecutionNode(ctx, _di);
		};
	}
}