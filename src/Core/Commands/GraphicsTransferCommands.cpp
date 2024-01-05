#include "GraphicsTransferCommands.hpp"


namespace vkl
{
	BlitImage::BlitImage(CreateInfo const& ci) :
		GraphicsTransferCommand(ci.app, ci.name),
		_src(ci.src),
		_dst(ci.dst),
		_regions(ci.regions),
		_filter(ci.filter)
	{}

	struct BlitImageNode : public ExecutionNode
	{
		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;
		BlitImageNode(CreateInfo const& ci):
			ExecutionNode(ExecutionNode::CI{
				.app = ci.app,
				.name = ci.name,
			})
		{}

		std::shared_ptr<ImageViewInstance> _src = nullptr;
		std::shared_ptr<ImageViewInstance> _dst = nullptr;
		MyVector<VkImageBlit2> _regions = {};
		VkFilter _filter = VK_FILTER_MAX_ENUM;

		void populate(BlitImage::BlitInfo const& bi)
		{
			_src = bi.src->instance();
			_dst = bi.dst->instance();
			_regions = bi.regions;
			_filter = bi.filter;

			resources() += ImageViewUsage{
				.ivi = _src,
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_READ_BIT,
					.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					.stage = VK_PIPELINE_STAGE_2_BLIT_BIT
				},
				.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			};

			resources() += ImageViewUsage{
				.ivi = _dst,
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.stage = VK_PIPELINE_STAGE_2_BLIT_BIT
				},
				.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			};
		}

		virtual void clear() override
		{
			ExecutionNode::clear();
			_src.reset();
			_dst.reset();
			_regions.clear();
			_filter = VK_FILTER_MAX_ENUM;
		}
		virtual void execute(ExecutionContext& ctx)
		{
			std::shared_ptr<CommandBuffer> cmd = ctx.getCommandBuffer();

			const VkImageBlit2* regions = _regions.data();
			uint32_t n_regions = _regions.size32();
			VkImageBlit2 _region;

			// Multi mip blit?

			if (n_regions == 0)
			{
				_region = VkImageBlit2{
					.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
					.pNext = nullptr,
					.srcSubresource = getImageLayersFromRange(_src->createInfo().subresourceRange),
					.srcOffsets = {makeZeroOffset3D(), convert(_src->image()->createInfo().extent)},
					.dstSubresource = getImageLayersFromRange(_dst->createInfo().subresourceRange),
					.dstOffsets = {makeZeroOffset3D(), convert(_dst->image()->createInfo().extent)},
				};
				regions = &_region;
				n_regions = 1;
			}

			VkBlitImageInfo2 info{
				.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
				.pNext = nullptr,
				.srcImage = _src->image()->handle(),
				.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				.dstImage = _dst->image()->handle(),
				.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.regionCount = n_regions,
				.pRegions = regions,
				.filter = _filter,
			};

			vkCmdBlitImage2(cmd->handle(), &info);
		}
	};

	std::shared_ptr<ExecutionNode> BlitImage::getExecutionNode(RecordContext& ctx, BlitInfo const& bi)
	{
		std::shared_ptr<BlitImageNode> node = _exec_node_cache.getCleanNode<BlitImageNode>([&]() {
			return std::make_shared<BlitImageNode>(BlitImageNode::CI{
				.app = application(),
				.name = name(),
			});
		});
		node->setName(name());
		node->populate(bi);
		return node;
	}

	std::shared_ptr<ExecutionNode> BlitImage::getExecutionNode(RecordContext& ctx)
	{
		return getExecutionNode(ctx, getDefaultBlitInfo());
	}

	Executable BlitImage::with(BlitInfo const& bi)
	{
		return [&](RecordContext& ctx)
		{
			BlitInfo _bi{
				.src = bi.src ? bi.src : _src,
				.dst = bi.dst ? bi.dst : _dst,
				.regions = bi.regions.empty() ? _regions : bi.regions,
				.filter = bi.filter == VK_FILTER_MAX_ENUM ? _filter : bi.filter,
			};
			return getExecutionNode(ctx, _bi);
		};
	}




	// [Vk]: [VL]: Validation Error: [ SYNC-HAZARD-WRITE-AFTER-READ ] Object 0: handle = 0xce045c0000000096, name = fabric_c.albedo_texture, type = VK_OBJECT_TYPE_IMAGE; | 
	// MessageID = 0x376bc9df | vkCmdPipelineBarrier2: Hazard WRITE_AFTER_READ for image barrier 2 VkImage 0xce045c0000000096[fabric_c.albedo_texture]. 
	// Access info (
	//	- usage: SYNC_IMAGE_LAYOUT_TRANSITION, 
	//	- prior_usage: SYNC_BLIT_TRANSFER_READ, 
	//	- read_barriers: VkPipelineStageFlags2(0), 
	//	- command: vkCmdBlitImage2, seq_no: 3, reset_no: 1).

	constexpr const VkPipelineStageFlagBits2 mips_blit_stage = VK_PIPELINE_STAGE_2_BLIT_BIT;


	struct ComputeMipsNode : public ExecutionNode
	{
		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;
		ComputeMipsNode(CreateInfo const& ci) :
			ExecutionNode(ExecutionNode::CI{
				.app = ci.app,
				.name = ci.name,
			})
		{}

		MyVector<AsynchMipsCompute> _targets = {};

		void populate(ComputeMips::ExecInfo const& ei)
		{
			_targets = ei.targets;



			for (size_t i = 0; i < _targets.size(); ++i)
			{
				resources() += ImageViewUsage{
					.ivi = _targets[i].target,
					.begin_state = {
						.access = VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT, // ??? Why transfer write (without it there is a synch validation hazard)
						.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						.stage = mips_blit_stage,
					},
					.usage = VK_IMAGE_USAGE_TRANSFER_BITS,
				};
			}
		}

		virtual void clear() override
		{
			ExecutionNode::clear();
			_targets.clear();
			targets.clear();
		}

		struct Target
		{
			ImageInstance* img;
			ImageViewInstance* view;
			uint32_t m;
			VkExtent3D extent;
		};
		MyVector<Target> targets;

		virtual void execute(ExecutionContext& ctx)
		{
			CommandBuffer& cmd = *ctx.getCommandBuffer();

			// Check targets are unique
			assert([&]() -> bool
			{
				bool res = true;
				std::set<ImageViewInstance*> set_views;
				for (auto& t : _targets)
				{
					set_views.insert(t.target.get());
				}
				res = set_views.size() == _targets.size();
				return res;
			}());

			
			targets.resize(_targets.size());

			uint32_t max_mip = 1;

			std::shared_ptr<CallbackHolder> completion_callbacks;

			for (size_t i = 0; i < _targets.size(); ++i)
			{
				targets[i] = Target{
					.img = _targets[i].target->image().get(),
					.view = _targets[i].target.get(),
					.m = _targets[i].target->createInfo().subresourceRange.levelCount,
					.extent = _targets[i].target->image()->createInfo().extent,
				};
				assert(targets[i].m > 1);
				max_mip = std::max(max_mip, targets[i].m);



				if (_targets[i].completion_callback.operator bool())
				{
					if (!completion_callbacks)
					{
						completion_callbacks = std::make_shared<CallbackHolder>();
					}
					completion_callbacks->callbacks.push_back(_targets[i].completion_callback);
				}
			}

			if (completion_callbacks)
			{
				ctx.keepAlive(std::move(completion_callbacks));
			}

			static thread_local MyVector<VkImageMemoryBarrier2> barriers;
			//barriers.reserve(targets.size() * 2);

			for (uint32_t m = 1; m < max_mip; ++m)
			{
				barriers.clear();

				for (size_t i = 0; i < targets.size(); ++i)
				{
					Target& tg = targets[i];
					if (tg.m > m)
					{
						// Write mip m
						barriers.push_back(VkImageMemoryBarrier2{
							.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
							.pNext = nullptr,
							.srcStageMask = mips_blit_stage,
							.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
							.dstStageMask = mips_blit_stage,
							.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
							.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
							.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
							.image = tg.img->handle(),
							.subresourceRange = {
								.aspectMask = tg.view->createInfo().subresourceRange.aspectMask,
								.baseMipLevel = m,
								.levelCount = 1,
								.baseArrayLayer = tg.view->createInfo().subresourceRange.baseArrayLayer,
								.layerCount = tg.view->createInfo().subresourceRange.layerCount,
							},
						});

						if (m > 1)
						{
							// Read mip m - 1
							barriers.push_back(VkImageMemoryBarrier2{
								.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
								.pNext = nullptr,
								.srcStageMask = mips_blit_stage,
								.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
								.dstStageMask = mips_blit_stage,
								.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
								.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
								.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
								.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
								.image = tg.img->handle(),
								.subresourceRange = {
									.aspectMask = tg.view->createInfo().subresourceRange.aspectMask,
									.baseMipLevel = m - 1,
									.levelCount = 1,
									.baseArrayLayer = tg.view->createInfo().subresourceRange.baseArrayLayer,
									.layerCount = tg.view->createInfo().subresourceRange.layerCount,
								},
							});
						}
					}
				}

				VkDependencyInfo dep = {
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

				vkCmdPipelineBarrier2(cmd, &dep);


				for (size_t i = 0; i < targets.size(); ++i)
				{
					Target& tg = targets[i];
					if (tg.m > m)
					{
						VkExtent3D smaller_extent = {
							.width = std::max(1u, tg.extent.width / 2),
							.height = std::max(1u, tg.extent.height / 2),
							.depth = std::max(1u, tg.extent.depth / 2),
						};

						VkImageBlit2 region = {
							.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
							.pNext = nullptr,
							.srcSubresource = {
								.aspectMask = tg.view->createInfo().subresourceRange.aspectMask,
								.mipLevel = m - 1,
								.baseArrayLayer = tg.view->createInfo().subresourceRange.baseArrayLayer,
								.layerCount = tg.view->createInfo().subresourceRange.layerCount,
							},
							.srcOffsets = {
								makeZeroOffset3D(), convert(tg.extent),
							},
							.dstSubresource = {
								.aspectMask = tg.view->createInfo().subresourceRange.aspectMask,
								.mipLevel = m,
								.baseArrayLayer = tg.view->createInfo().subresourceRange.baseArrayLayer,
								.layerCount = tg.view->createInfo().subresourceRange.layerCount,
							},
							.dstOffsets = {
								makeZeroOffset3D(), convert(smaller_extent),
							},
						};

						VkBlitImageInfo2 blit{
							.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
							.pNext = nullptr,
							.srcImage = tg.img->handle(),
							.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							.dstImage = tg.img->handle(),
							.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							.regionCount = 1,
							.pRegions = &region,
							.filter = VK_FILTER_LINEAR,
						};

						vkCmdBlitImage2(cmd, &blit);

						tg.extent = smaller_extent;
					}
				}
			}

			// One last barriers
			barriers.resize(targets.size());
			for (size_t i = 0; i < targets.size(); ++i)
			{
				Target& tg = targets[i];
				barriers[i] = VkImageMemoryBarrier2{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
					.pNext = nullptr,
					.srcStageMask = mips_blit_stage,
					.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.dstStageMask = mips_blit_stage,
					.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = tg.img->handle(),
					.subresourceRange = {
						.aspectMask = tg.view->createInfo().subresourceRange.aspectMask,
						.baseMipLevel = tg.m - 1,
						.levelCount = 1,
						.baseArrayLayer = tg.view->createInfo().subresourceRange.baseArrayLayer,
						.layerCount = tg.view->createInfo().subresourceRange.layerCount,
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
				.imageMemoryBarrierCount = barriers.size32(),
				.pImageMemoryBarriers = barriers.data(),
			};
			vkCmdPipelineBarrier2(cmd, &dep);
		}
	};

	std::shared_ptr<ExecutionNode> ComputeMips::getExecutionNode(RecordContext& ctx, ExecInfo const& ei)
	{
		std::shared_ptr<ComputeMipsNode> node = _exec_node_cache.getCleanNode<ComputeMipsNode>([&]() {
			return std::make_shared<ComputeMipsNode>(ComputeMipsNode::CI{
				.app = application(),
				.name = name(),
			});
		});

		node->setName(name());
		node->populate(ei);
		return node;
	}

	std::shared_ptr<ExecutionNode> ComputeMips::getExecutionNode(RecordContext& ctx)
	{
		return getExecutionNode(ctx, getDefaultExecInfo());
	}

	Executable ComputeMips::with(ExecInfo const& ei)
	{
		return [this, ei](RecordContext& ctx)
		{
			return getExecutionNode(ctx, ei);
		};
	}










	ClearImage::ClearImage(CreateInfo const& ci) :
		GraphicsTransferCommand(ci.app, ci.name),
		_view(ci.view),
		_value(ci.value)
	{

	}

	struct ClearImageNode : public ExecutionNode
	{
		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;
		ClearImageNode(CreateInfo const& ci) :
			ExecutionNode(ExecutionNode::CI{
				.app = ci.app,
				.name = ci.name,
				})
		{}

		std::shared_ptr<ImageViewInstance> _target = nullptr;
		VkClearValue _value = {};

		void populate(ClearImage::ClearInfo const& ci)
		{
			_target = ci.view->instance();
			_value = ci.value.value();

			resources() += ImageViewUsage{
				.ivi = _target,
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.stage = VK_PIPELINE_STAGE_2_CLEAR_BIT,
				},
				.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			};
		}

		virtual void clear()
		{
			ExecutionNode::clear();
			_target.reset();
			_value = {};
		}

		virtual void execute(ExecutionContext& ctx)
		{
			std::shared_ptr<CommandBuffer> cmd = ctx.getCommandBuffer();

			const VkImageSubresourceRange& range = _target->createInfo().subresourceRange;

			const VkClearValue value = _value;

			VkImageAspectFlags aspect = range.aspectMask;

			// TODO manage other aspects
			if (aspect & VK_IMAGE_ASPECT_COLOR_BIT)
			{
				vkCmdClearColorImage(
					*cmd,
					_target->image()->handle(),
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
					_target->image()->handle(),
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					&value.depthStencil,
					1,
					&range
				);
			}
		}
	};

	std::shared_ptr<ExecutionNode> ClearImage::getExecutionNode(RecordContext& ctx, ClearInfo const& ci)
	{
		std::shared_ptr<ClearImageNode> node = _exec_node_cache.getCleanNode<ClearImageNode>([&]() {
			return std::make_shared<ClearImageNode>(ClearImageNode::CI{
				.app = application(),
				.name = name(),
			});
		});
		node->setName(name());
		node->populate(ci);
		return node;
	}

	std::shared_ptr<ExecutionNode> ClearImage::getExecutionNode(RecordContext& ctx)
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
}