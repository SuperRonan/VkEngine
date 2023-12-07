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

		ImageViewInstance& view = *ei.target->instance();
		ImageInstance& img = *ei.target->image()->instance();

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
				.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
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
					.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
					.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
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
				.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
				.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
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

		ctx.keppAlive(ei.target->instance());
	}

	ExecutionNode ComputeMips::getExecutionNode(RecordContext& ctx, ExecInfo const& ei)
	{
		Resources resources{
			Resource{
				._image = ei.target,
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
			},
		};
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
				ExecInfo _ei{
					.target = ei.target ? ei.target : _target,
				};
				return getExecutionNode(ctx, _ei);
			};
	}
}