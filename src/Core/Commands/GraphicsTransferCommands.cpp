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

	void BlitImage::execute(ExecutionContext& context, BlitInfo const& bi)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();

		const VkImageBlit* regions = bi.regions.data();
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
		context.keppAlive(bi.src->instance());
		context.keppAlive(bi.dst->instance());
	}

	ExecutionNode BlitImage::getExecutionNode(RecordContext& ctx, BlitInfo const& bi)
	{
		Resources resources = {
			Resource{
				._image = bi.src,
				._begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_READ_BIT,
					.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					.stage = VK_PIPELINE_STAGE_2_BLIT_BIT
				},
				._image_usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			},
			Resource{
				._image = bi.dst,
				._begin_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.stage = VK_PIPELINE_STAGE_2_BLIT_BIT
				},
				._image_usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			},
		};
		BlitInfo bi_copy = bi;
		ExecutionNode res = ExecutionNode::CI{
			.name = name(),
			.resources = resources,
			.exec_fn = [this, bi_copy](ExecutionContext& ctx)
			{
				execute(ctx, bi_copy);
			},
		};
		return res;
	}

	ExecutionNode BlitImage::getExecutionNode(RecordContext& ctx)
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


	void ComputeMips::execute(ExecutionContext& ctx, ExecInfo const& ei)
	{
		CommandBuffer& cmd = *ctx.getCommandBuffer();

		assert([&]() -> bool
		{
			bool res = true;
			std::set<ImageView*> set_views;
			for (auto& t : ei.targets)
			{
				set_views.insert(t.get());
			}
			res = set_views.size() == ei.targets.size();
			return res;
		}());

		struct Target
		{
			ImageInstance * img;
			ImageViewInstance * view;
			uint32_t m;
			VkExtent3D extent;
		};
		std::vector<Target> targets(ei.targets.size());
		
		uint32_t max_mip = 1;
		for (size_t i = 0; i < ei.targets.size(); ++i)
		{
			targets[i] = Target{
				.img = ei.targets[i]->instance()->image().get(),
				.view = ei.targets[i]->instance().get(),
				.m = ei.targets[i]->instance()->createInfo().subresourceRange.baseMipLevel,
				.extent = ei.targets[i]->instance()->image()->createInfo().extent,
			};
			assert(targets[i].m > 1);
			max_mip = std::max(max_mip, targets[i].m);
		}


		std::vector<VkImageMemoryBarrier2> barriers;
		barriers.reserve(targets.size() * 2);

		for(uint32_t m=1; m < max_mip; ++m)
		{
			barriers.clear();

			for (size_t i = 0; i < targets.size(); ++i)
			{
				Target & tg = targets[i];
				if (tg.m > m)
				{
					barriers.push_back(VkImageMemoryBarrier2{
						.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
						.pNext = nullptr,
						.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
						.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
						.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
						.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
						.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.image = tg.img->handle(),
						.subresourceRange = {
							.aspectMask = tg.view->createInfo().subresourceRange.aspectMask,
							.baseMipLevel = tg.m,
							.levelCount = 1,
							.baseArrayLayer = tg.view->createInfo().subresourceRange.baseArrayLayer,
							.layerCount = tg.view->createInfo().subresourceRange.layerCount,
						},
					});

					if (m > 1)
					{
						barriers.push_back(VkImageMemoryBarrier2{
							.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
							.pNext = nullptr,
							.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
							.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
							.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
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
				.imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size()),
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
							.mipLevel = tg.m - 1,
							.baseArrayLayer = tg.view->createInfo().subresourceRange.baseArrayLayer,
							.layerCount = tg.view->createInfo().subresourceRange.layerCount,
						},
						.srcOffsets = {
							makeZeroOffset3D(), convert(tg.extent),
						},
						.dstSubresource = {
							.aspectMask = tg.view->createInfo().subresourceRange.aspectMask,
							.mipLevel = tg.m,
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
				.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
				.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
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
			.imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size()),
			.pImageMemoryBarriers = barriers.data(),
		};
		vkCmdPipelineBarrier2(cmd, &dep);
	}

	ExecutionNode ComputeMips::getExecutionNode(RecordContext& ctx, ExecInfo const& ei)
	{
		Resources resources;
		resources.resize(ei.targets.size());
		for (size_t i = 0; i < resources.size(); ++i)
		{
			resources[i] = Resource{
				._image = ei.targets[i],
				._begin_state = {
					.access = VK_ACCESS_2_TRANSFER_READ_BIT,
					.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					.stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
				},
				._end_state = ResourceState2{
					.access = VK_ACCESS_2_TRANSFER_READ_BIT,
					.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					.stage = VK_PIPELINE_STAGE_2_BLIT_BIT,
				},
				._image_usage = VK_IMAGE_USAGE_TRANSFER_BITS,
			};
		}
		ExecInfo ei_copy = ei;
		ExecutionNode res = ExecutionNode::CI{
			.name = name(),
			.resources = resources,
			.exec_fn = [this, ei_copy](ExecutionContext& ctx) {
				execute(ctx, ei_copy);
			},
		};
		return res;
	}

	ExecutionNode ComputeMips::getExecutionNode(RecordContext& ctx)
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
}